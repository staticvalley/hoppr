#define SDL_MAIN_USE_CALLBACKS 1

#define DEBUG 1

#include <glad/glad.h>

#include <print>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_main.h>

// debug library
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <Player.hpp>
#include <Shader.hpp>
#include <Camera.hpp>
#include <Mesh.hpp>
#include <Model.hpp>
#include <Skybox.hpp>
#include <Lighting.hpp>
#include <Collision.hpp>
#include <BSPMap.hpp>

#include <DebugDrawer.hpp>
#include <DebugUtils.hpp>

#define WINDOW_WIDTH 800.0f
#define WINDOW_HEIGHT 600.0f

SDL_Window* window = nullptr;
SDL_GLContext opengl_context = nullptr;
Shader* basic;
Skybox* skybox;
Player* player;
PointLight pl;
GLfloat lastTime = 0.0f;
BSPMap* map;
DirectionalLight sun;
glm::vec3 spawnPos;

bool debugHudOn = false;
bool lightOn = true;

void drawDebugHud();
FPSCounter fps;

// program initialization
SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {

    // initialize sdl video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("sdl3 video subsystem could not be initialized: %s",
            SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // set opengl version and use core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // create sdl window
    window = SDL_CreateWindow(
        "opengl_fps ver. 1", WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Log("sdl3 window creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // create context and attach to sdl window
    opengl_context = SDL_GL_CreateContext(window);
    if (!opengl_context) {
        SDL_Log("sdl3 opengl context creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // load glad gl functions
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("glad failed to initialize");
        return SDL_APP_FAILURE;
    }

    // init sdl event subsystem
    if (!SDL_Init(SDL_INIT_EVENTS)) {
        SDL_Log("sdl3 event subsystem could not be initialized: %s",
            SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // enable z testing for drawing
    glEnable(GL_DEPTH_TEST);

    // hide mouse when window is focused
    SDL_SetWindowRelativeMouseMode(window, true);

#ifdef DEBUG
    // create imgui context and bind to sdl3 window
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    style.ScaleAllSizes(main_scale);        // bake a fixed style scale
    style.FontScaleDpi = main_scale;        // set initial font scale

    ImGui_ImplSDL3_InitForOpenGL(window, opengl_context);
    ImGui_ImplOpenGL3_Init("#version 460");

    DebugDrawer::init();

#endif

    map = new BSPMap("assets/game/cs_assault.bsp");

    map->nullTexture = new Texture("assets/textures/missing_content.png");

    basic = new Shader("assets/shaders/phong.vert", "assets/shaders/phong.frag");

    player = nullptr;
    for (const BSPEntity& entity : map->bsp.entities()) {
        if (entity.at("classname") == "info_player_start") {
            std::string origin = entity.at("origin");
            float x, y, z;
            sscanf(origin.c_str(), "%f %f %f", &x, &y, &z);
            spawnPos = glm::vec3(x, z, -y);
            SDL_Log("spawn point found: %.2f %.2f %.2f", spawnPos.x, spawnPos.y, spawnPos.z);
            player = new Player(spawnPos, 90.0f, WINDOW_WIDTH / WINDOW_HEIGHT);
            break;
        }
    }
    // fallback if no spawn found
    if (!player) {
        SDL_Log("no spawn point found, using default");
        player = new Player(glm::vec3(0, 0, 0), 90.0f, WINDOW_WIDTH / WINDOW_HEIGHT);
    }

    sun.ambient_color = glm::vec3(1.0f, 1.0f, 1.0f);

    pl.position = glm::vec3(0, 0, 0);
    pl.ambient_color = glm::vec3(0.2);
    pl.diffuse_color = glm::vec3(0.9);

    return SDL_APP_CONTINUE;
}

// main program loop
SDL_AppResult SDL_AppIterate(void* appstate) {

    // calculate delta time
    float currentTime = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    player->update(deltaTime, *map);

    // clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    basic->bind();

    // camera uniforms
    basic->setUniformMat4("u_view", player->camera->getViewMatrix());
    basic->setUniformMat4("u_projection", player->camera->getProjectionMatrix());
    basic->setUniformVec3("u_camera_position", player->camera->getPosition());

    // light uniforms
    basic->setUniformInt1("u_spot_light_count", 0);
    basic->setUniformBool1("u_has_directional_light", true);

    sun.apply(*basic);

    pl.position = glm::vec3(0, sin(currentTime) + 0.5, -0.5);

    if (lightOn) {
        basic->setUniformInt1("u_point_light_count", 1);
        pl.apply(*basic, 0);
    }
    else {
        basic->setUniformInt1("u_point_light_count", 0);
    }

    map->draw(*basic, *(player->camera));

    if (debugHudOn) {
        fps.tick();
        drawDebugHud();
    }

    // switch buffer to screen
    SDL_GL_SwapWindow(window);

    return SDL_APP_CONTINUE;
}

// event handler
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        player->processMouse(event->motion.xrel, event->motion.yrel);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        // set new aspect ratio for projection
        player->camera->updateAspectRatio((float)event->window.data1, (float) event->window.data2);
        glViewport(0, 0, event->window.data1, event->window.data2);
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.scancode == SDL_SCANCODE_F) lightOn = !lightOn;
        if (event->key.scancode == SDL_SCANCODE_F3) debugHudOn = !debugHudOn;
        if (event->key.scancode == SDL_SCANCODE_N) {
            if (player->movement.moveType == MovementType::WALK) {
                player->movement.moveType = MovementType::NOCLIP;
            } else {
                player->movement.moveType = MovementType::WALK;
            }
        }
        break;
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

// no cleanup needed, sdl cleans up its own entities
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    delete player;
    delete basic;
#ifdef DEBUG
    DebugDrawer::cleanup();
#endif 
    SDL_GL_DestroyContext(opengl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void drawDebugHud() {

    glm::vec3 cameraPosition = player->camera->getPosition();
    glm::vec3 cameraForward = player->camera->getForwardDirection();

    glm::vec3 velocity = player->movement.velocity;
    glm::vec3 origin = player->movement.origin;
    DebugDrawer::draw3DGizmo(*(player->camera));
    //for (SceneObject& obj : sceneObjectList)
    //    DebugDrawer::drawAABB(*(player->camera), obj.model->boundingBox);

    DebugDrawer::drawPoint(*(player->camera), origin, glm::vec3(0, 1, 1), 0.5);
    DebugDrawer::drawPoint(*(player->camera), glm::vec3(0,0,0), glm::vec3(0, 0, 1), 0.5);
    DebugDrawer::drawPoint(*(player->camera), spawnPos, glm::vec3(1, 0, 0), 0.5);
    DebugDrawer::drawPoint(*(player->camera), glm::vec3(spawnPos.x, 0, spawnPos.z), glm::vec3(0, 1, 0), 0.5);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(0, 0));

    ImGui::Begin("Debug");
    ImGui::Text("camera pos [%.2lf %.2lf %.2lf]", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    ImGui::Text("camera view [%.2lf %.2lf %.2lf]", cameraForward.x, cameraForward.y, cameraForward.z);
    ImGui::Text("origin [%.2lf %.2lf %.2lf]", origin.x, origin.y, origin.z);
    ImGui::Text("velocity [%.2lf %.2lf %.2lf]", velocity.x, velocity.y, velocity.z);
    ImGui::Text(fps.fpsString.c_str());
    ImGui::End();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}