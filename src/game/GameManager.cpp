#include <GameManager.hpp>
#include <SDL3/SDL.h>
#include <DebugDraw.hpp>
#include <MDLAsset.hpp>
#include <Util.hpp>

GameManager::GameManager(const GameManagerInitConfig& config) {

    input.setBindsFromFile(config.bindConfigPath);

    map = new BSPMap(config.mapPath);
    map->nullTexture = new Texture(config.nullTexturePath);

    worldShader = new Shader("world (phong)", config.worldVertShaderPath, config.worldFragShaderPath);
    flatShader = new Shader("flat", config.worldVertShaderPath, "assets/shaders/debug/flat.frag");

    // get world spawn
    bool foundSpawn = false;
    for (const BSPEntity& entity : map->bsp.entities()) {
        if (entity.at("classname") == "info_player_start") {
            std::string origin = entity.at("origin");
            float x, y, z;
            sscanf(origin.c_str(), "%f %f %f", &x, &y, &z);
            worldSpawn = glm::vec3(x, z, -y);
            Log::info(Log::Channel::MAPPING,"player spawn point found: {}",worldSpawn);
            foundSpawn = true;
            break;
        }
    }

    if (!foundSpawn) {
        worldSpawn = glm::vec3(0.0f);
        Log::info(Log::Channel::MAPPING, "no spawn point found, using default {}", worldSpawn);
    }

    // init player
    player = new Player(worldSpawn, config.playerFOV, config.baseWindowWidth / config.baseWindowHeight);

    worldLight.ambient_color = glm::vec3(1.0f, 1.0f, 1.0f);

    character = loadMDL("assets/models/character.mdl");
    if (character) {
        character->model.position = worldSpawn;
    }
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

    DebugDraw::menuInfo("View Forward", "{:.2f}", player->camera->getForwardDirection());
    DebugDraw::menuInfo("Origin", "{:.2f}", player->movement.origin);
    DebugDraw::menuInfo("Speed", "{:.2f}", player->movement.getSpeed());
}

void GameManager::render() {
    // clear color and depth buffers
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    worldShader->bind();
    applyPhongUniforms(*worldShader);

    map->draw(*worldShader, *(player->camera));

    if (character) {
        character->model.updateBoundingBox();
        character->model.draw(*worldShader);
        DebugDraw::boundingBox(character->model.boundingBox);
    }

    worldShader->unbind();
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

void GameManager::applyPhongUniforms(Shader& shader) {
    // camera uniforms
    shader.setUniformMat4("u_view", player->camera->getViewMatrix());
    shader.setUniformMat4("u_projection", player->camera->getProjectionMatrix());
    shader.setUniformVec3("u_camera_position", player->camera->getPosition());

    // light uniforms
    shader.setUniformInt1("u_spot_light_count", 0);
    shader.setUniformInt1("u_point_light_count", 0);
    shader.setUniformBool1("u_has_directional_light", true);

    worldLight.apply(shader);
}
