#include <Player.hpp>
#include <Camera.hpp>
#include <Model.hpp>
#include <Collision.hpp>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <algorithm>

Player::Player(glm::vec3 origin, GLfloat fov, GLfloat aspectRatio) {
	movement.origin = origin;
	//movement.origin.y += PM_PLAYER_ORIGIN;
	camera = new Camera(
		movement.origin + glm::vec3(0.0f, PM_EYEHEIGHT_FROM_ORIGIN, 0.0f), 
		fov, 
		aspectRatio
	);
	updateCollisionBox();
}

Player::~Player() {
	delete camera;
}

void Player::update(float dt, BSPMap& map) {

	// flatten forward and right for more fps style movement
	glm::vec3 forward = camera->getForwardDirection();
	glm::vec3 flatForward = glm::vec3(forward.x, 0.0f, forward.z);

	// guard against NaN normalization
	if (glm::length(flatForward) > 0.001f)
		flatForward = glm::normalize(flatForward);

	glm::vec3 right = camera->getRightDirection();
	glm::vec3 flatRight = glm::vec3(right.x, 0.0f, right.z);

	// guard against NaN normalization
	if (glm::length(flatRight) > 0.001f)
		flatRight = glm::normalize(flatRight);
	
	// get keyboard inputs
	const bool* keyInputs = SDL_GetKeyboardState(nullptr);

	glm::vec3 movementInput(0.0f);

	if (keyInputs[SDL_SCANCODE_W]) movementInput += flatForward;
	if (keyInputs[SDL_SCANCODE_S]) movementInput -= flatForward;
	if (keyInputs[SDL_SCANCODE_A]) movementInput -= flatRight;
	if (keyInputs[SDL_SCANCODE_D]) movementInput += flatRight;

	// normalize so diagonal isn't faster
	if (glm::length(movementInput) > 0.001f)
		movementInput = glm::normalize(movementInput);

	if (movement.moveType == MovementType::WALK) {
		// poll for jump
		movement.jumpPressed = keyInputs[SDL_SCANCODE_SPACE];
		// poll for crouch
		movement.crouchPressed = keyInputs[SDL_SCANCODE_LCTRL];
		// poll for sneak
		movement.isSneaking = keyInputs[SDL_SCANCODE_LSHIFT];
	}

	if (movement.moveType == MovementType::NOCLIP) {
		if (keyInputs[SDL_SCANCODE_SPACE]) movementInput += glm::vec3(0.0f, 1.0f, 0.0f);
		if (keyInputs[SDL_SCANCODE_LSHIFT]) movementInput -= glm::vec3(0.0f, 1.0f, 0.0f);
	}

	movement.update(dt, movementInput, map);

	updateCamera();

	updateCollisionBox();
}

void Player::updateCamera() {
	// camera is PM_EYEHEIGHT units from the feet of the model
	if (movement.isDucking) {
		camera->updatePosition(movement.origin + glm::vec3(0.0f, PM_EYEHEIGHT_FROM_ORIGIN_CROUCHED, 0.0f));
	} else {
		camera->updatePosition(movement.origin + glm::vec3(0.0f, PM_EYEHEIGHT_FROM_ORIGIN, 0.0f));
	}
}

void Player::processMouse(float x, float y) {
	camera->updateViewXRelYRel(x, y);
}

void Player::updateCollisionBox() {
	/*collisionBox = AABB::createFromPoint(
		movement.origin + glm::vec3(0.0f, PM_PLAYER_ORIGIN, 0.0f),
		glm::vec3(0.5f, PM_PLAYER_HEIGHT, 0.5f)
	);*/
}