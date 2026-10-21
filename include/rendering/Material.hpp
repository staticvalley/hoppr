#pragma once

#include <memory>
#include <Texture.hpp>
#include <Shader.hpp>

class Material {
public:
	std::shared_ptr<Texture> diffuseTexture = nullptr;
	
	// applies active texture for drawing meshes
	void apply(Shader& shader);
};