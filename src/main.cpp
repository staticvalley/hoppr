#define SDL_MAIN_USE_CALLBACKS 1

#include <glad/glad.h>

#include <print>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// debug library
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <Player.hpp>
#include <Lighting.hpp>
#include <GameManager.hpp>

#include <DebugDraw.hpp>
#include <DebugUtils.hpp>
#include <Log.hpp>

#define WINDOW_WIDTH 800.0f
#define WINDOW_HEIGHT 600.0f

#define MAP_NAME "cs_assault"

SDL_Window* window = nullptr;
SDL_GLContext opengl_context = nullptr;
GLfloat lastTime = 0.0f;
GameManager* game;

//void drawDebugHud();
FPSCounter fps;

// program initialization
SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    // silence unused warnings
    (void)appstate;
    (void)argc;
    (void)argv;

    Log::init(spdlog::level::trace);

    // initialize sdl video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        Log::critical(Log::Channel::ENGINE, "Video subsystem could not be initialized: {}", SDL_GetError());
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
        Log::critical(Log::Channel::ENGINE, "Window creation failed: {}", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // create context and attach to sdl window
    opengl_context = SDL_GL_CreateContext(window);
    if (!opengl_context) {
        Log::critical(Log::Channel::ENGINE, "Opengl context creation failed: {}", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // load glad gl functions
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        Log::critical(Log::Channel::ENGINE, "Glad failed to initialize");
        return SDL_APP_FAILURE;
    }

    // init sdl event subsystem
    if (!SDL_Init(SDL_INIT_EVENTS)) {
        Log::critical(Log::Channel::ENGINE, "Event subsystem could not be initialized: {}", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // enable z testing for drawing
    glEnable(GL_DEPTH_TEST);

    // hide mouse when window is focused
    SDL_SetWindowRelativeMouseMode(window, true);

    GameManagerInitConfig config = {
        .baseWindowWidth    = WINDOW_WIDTH,
        .baseWindowHeight   = WINDOW_HEIGHT,

        .playerFOV  = 90.0f,

        .mapPath    = "assets/game/" MAP_NAME ".bsp",

        .nullTexturePath        = "assets/textures/missing_content.png",
        .worldVertShaderPath    = "assets/shaders/phong.vert",
        .worldFragShaderPath    = "assets/shaders/phong.frag",

        .bindConfigPath = "assets/config/binds.txt",
    };

    game = new GameManager(config);

#if 1
    // create imgui context and bind to sdl3 window
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    style.ScaleAllSizes(main_scale);        // bake a fixed style scale
    style.FontScaleDpi = main_scale;        // set initial font scale

    ImGui_ImplSDL3_InitForOpenGL(window, opengl_context);
    ImGui_ImplOpenGL3_Init("#version 460");

    DebugDraw::init(game->player->camera);
    DebugDraw::setEnabled(true);

#endif

    return SDL_APP_CONTINUE;
}

// main program loop
SDL_AppResult SDL_AppIterate(void* appstate) {
    // silence unused warnings
    (void)appstate;

    // calculate delta time
    float currentTime = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    game->update(deltaTime);
    game->render();

    if (game->debugModeOn) {
        DebugDraw::orientationGizmo();
        // drawDebugHud();
        fps.tick();
    }

    DebugDraw::flush();

    // switch buffer to screen
    SDL_GL_SwapWindow(window);

    return SDL_APP_CONTINUE;
}

// event handler
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    // silence unused warnings
    (void)appstate;
    return game->handleEvent(event);
}

// no cleanup needed, sdl cleans up its own entities
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    // silence unused warnings
    (void)appstate;
    (void)result;
    delete game;
#ifdef DEBUG
    DebugDraw::cleanup();
#endif 
    SDL_GL_DestroyContext(opengl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/*
void drawDebugHud() {

    glm::vec3 cameraPosition = player->camera->getPosition();
    glm::vec3 cameraForward = player->camera->getForwardDirection();

    glm::vec3 velocity = player->movement.velocity;
    float speed = player->movement.getSpeed();
    glm::vec3 origin = player->movement.origin;
    DebugDraw::orientationGizmo();
    //for (SceneObject& obj : sceneObjectList)
    //    DebugDrawer::drawAABB(*(player->camera), obj.model->boundingBox);

    DebugDraw::point(origin, glm::vec3(0, 1, 1), 0.5);
    DebugDraw::point(glm::vec3(0,0,0), glm::vec3(0, 0, 1), 0.5);
    DebugDraw::point(spawnPos, glm::vec3(1, 0, 0), 0.5);
    DebugDraw::point(glm::vec3(spawnPos.x, 0, spawnPos.z), glm::vec3(0, 1, 0), 0.5);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(0, 0));

    ImGui::Begin("Debug");
    ImGui::Text("camera pos [%.2lf %.2lf %.2lf]", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    ImGui::Text("camera view [%.2lf %.2lf %.2lf]", cameraForward.x, cameraForward.y, cameraForward.z);
    ImGui::Text("origin [%.2lf %.2lf %.2lf]", origin.x, origin.y, origin.z);
    ImGui::Text("speed %.2lf", speed);
    ImGui::Text("velocity [%.2lf %.2lf %.2lf]", velocity.x, velocity.y, velocity.z);
    ImGui::Text(fps.fpsString.c_str());
    ImGui::End();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}*/