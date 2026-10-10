#pragma once

#include <Camera.hpp>
#include <Model.hpp>
#include <Input.hpp>
#include <BSPMap.hpp>
#include <PlayerMovement.hpp>

class Player {
public:

	Player(glm::vec3 position, GLfloat fov, GLfloat aspectRatio);
	~Player();

	void update(GLfloat dt, Input input, BSPMap& map);
	void processMouse(GLfloat x, GLfloat y);
	void updateCollisionBox();

	void updatePos(glm::vec3 newp) { movement.origin = newp; }

	const glm::vec3& position() { return movement.origin; }
	const glm::vec3& velocity() { return movement.velocity; }

	Camera* camera;
	AABB collisionBox;

	PlayerMovement movement{};

private:

	float health = 100.0f;

	void updateCamera();

};