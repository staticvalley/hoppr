#define SDL_MAIN_USE_CALLBACKS 1

#include <glad/glad.h>

#include <print>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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

    const GameManagerInitConfig config = {
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

    DebugDraw::init(DebugDraw::InitContext{
        .window = window,
        .glContext = opengl_context,
        .baseCamera = game->player->camera
    });

    DebugDraw::setDebugMenuEnabled(true);

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
        fps.tick();
        DebugDraw::menuInfo("FPS", "{}", fps.fpsString.c_str());
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
    DebugDraw::cleanup();
    SDL_GL_DestroyContext(opengl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}