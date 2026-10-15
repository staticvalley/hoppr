#pragma once
#include <SDL3/SDL.h>
#include <BSPMap.hpp>
#include <Player.hpp>
#include <Input.hpp>
#include <Lighting.hpp>

struct GameManagerInitConfig {
    // base window sizings
    float baseWindowWidth;
    float baseWindowHeight;

    // base character settings
    float playerFOV;

    // map settings
    const char* mapPath;

    // base rendering asset paths
    const char* nullTexturePath;
    const char* worldVertShaderPath;
    const char* worldFragShaderPath;

    // config paths
    const char* bindConfigPath;
};

class GameManager {
public:
    GameManager(const GameManagerInitConfig& config);
    ~GameManager();

    void update(float dt);
    SDL_AppResult handleEvent(SDL_Event* event);
    void render();

    bool debugModeOn            = false;

    Input input;
    Player *player              = nullptr;
    BSPMap *map                 = nullptr;

    Shader *worldShader         = nullptr;
    DirectionalLight worldLight;

    glm::vec3 worldSpawn        = glm::vec3(0.0f, 0.0f, 0.0f);
};