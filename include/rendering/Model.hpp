#pragma once

#include <Mesh.hpp>
#include <Shader.hpp>
#include <Collision.hpp>

#include <glm/glm.hpp>
#include <vector>

#include "Log.hpp"

class Model {
public:

	std::string name;

	Model(std::string name);

	// note: this function moves mesh data ownership from argument into Model
	void addMesh(Mesh mesh);

	void setTexture(const std::shared_ptr<Texture>& texture);

	void draw(Shader& shader);

	static Model* loadOBJ(const char* filePath);

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale	= glm::vec3(1.0f);

	// model uses radians for transform, convert from degrees
	void setRotationDegrees(glm::vec3 rotationTransform);

	glm::mat4 getModelMatrix();

	AABB boundingBox;

	// set bounding box max and min coords
	void updateBoundingBox();

private:

	// vector of meshes for model
	std::vector<Mesh> meshList;

};
