#include <GameManager.hpp>
#include <SDL3/SDL.h>
#include <DebugDraw.hpp>

GameManager::GameManager(const GameManagerInitConfig& config) {

    input.setBindsFromFile(config.bindConfigPath);

    map = new BSPMap(config.mapPath);
    map->nullTexture = new Texture(config.nullTexturePath);

    worldShader = new Shader(config.worldVertShaderPath, config.worldFragShaderPath);
    Log::info(Log::Channel::RENDERER, "Loaded default shader (this is hardcoded, fix?).");

    // get world spawn
    bool foundSpawn = false;
    for (const BSPEntity& entity : map->bsp.entities()) {
        if (entity.at("classname") == "info_player_start") {
            std::string origin = entity.at("origin");
            float x, y, z;
            sscanf(origin.c_str(), "%f %f %f", &x, &y, &z);
            worldSpawn = glm::vec3(x, z, -y);
            Log::info(
                Log::Channel::MAPPING,
                "Player spawn point found: [x: {}, y: {}, z: {}]",
                worldSpawn.x,
                worldSpawn.y,
                worldSpawn.z
                );
            foundSpawn = true;
            break;
        }
    }

    if (!foundSpawn) {
        Log::info(Log::Channel::MAPPING, "No spawn point found, using default");
        worldSpawn = glm::vec3(0.0f);
    }

    // init player
    player = new Player(worldSpawn, config.playerFOV, config.baseWindowWidth / config.baseWindowHeight);

    worldLight.ambient_color = glm::vec3(1.0f, 1.0f, 1.0f);

}

GameManager::~GameManager() {
    delete player;
    delete worldShader;
    delete map;
}

void GameManager::update(float dt) {

    input.snapshotActionStates();

    player->update(dt, input, *map);

    if (input.pressed(Action::DEBUG)) {
        Log::info(Log::Channel::GAME, "Debug mode = {}", debugModeOn ? "ON" : "OFF");
        debugModeOn = !debugModeOn;
        DebugDraw::setEnabled(debugModeOn);
    }

    input.resetMouseWheelPoll();
}

void GameManager::render() {
    // clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    worldShader->bind();

    // camera uniforms
    worldShader->setUniformMat4("u_view", player->camera->getViewMatrix());
    worldShader->setUniformMat4("u_projection", player->camera->getProjectionMatrix());
    worldShader->setUniformVec3("u_camera_position", player->camera->getPosition());

    // light uniforms
    worldShader->setUniformInt1("u_spot_light_count", 0);
    worldShader->setUniformInt1("u_point_light_count", 0);
    worldShader->setUniformBool1("u_has_directional_light", true);

    worldLight.apply(*worldShader);

    map->draw(*worldShader, *(player->camera));
}

SDL_AppResult GameManager::handleEvent(SDL_Event* event) {
    input.pollMouseWheel(event);
    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION:
            player->processMouse(event->motion.xrel, event->motion.yrel);
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            // set new aspect ratio for projection
            player->camera->updateAspectRatio((float)event->window.data1, (float) event->window.data2);
            glViewport(0, 0, event->window.data1, event->window.data2);
            break;
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}
