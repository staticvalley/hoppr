#pragma once

#include <SDL3/SDL.h>

struct FPSCounter {
    uint32_t frameCount = 0;
    float currentTime = 0;
    float lastTime = (float)SDL_GetTicks() / 1000.0f;
    // holds string representation
    std::string fpsString = "inf ms/frame";

    void tick() {
        currentTime = (float)SDL_GetTicks() / 1000.0f;
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            fpsString = std::format("{:.2} ms/frame", 1000.0f/(double)frameCount);
            frameCount = 0;
            lastTime += 1.0f;
        }
    }
};