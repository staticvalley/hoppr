#pragma once

#include <Model.hpp>
#include <Texture.hpp>
#include <memory>
#include <vector>

/// @todo possibly something i should ditch in the future (or ditch model.hpp?)
struct MDLAsset {
    Model model;
    std::vector<std::shared_ptr<Texture>> textures;
};

std::unique_ptr<MDLAsset> loadMDL(const char* path);