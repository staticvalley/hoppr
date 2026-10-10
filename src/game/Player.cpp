#include <Player.hpp>
#include <Camera.hpp>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <Input.hpp>

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

void Player::update(float dt, Input input, BSPMap& map) {

	movement.forwardMove = 0.0f;
	movement.rightMove   = 0.0f;
	movement.upMove      = 0.0f;

	if (input.poll(Action::MOVE_FORWARD))	movement.forwardMove	+= 1.0f;
	if (input.poll(Action::MOVE_BACKWARD))	movement.forwardMove	-= 1.0f;
	if (input.poll(Action::MOVE_RIGHT))		movement.rightMove		+= 1.0f;
	if (input.poll(Action::MOVE_LEFT))		movement.rightMove		-= 1.0f;

	if (movement.moveType == MovementType::WALK) {
		// poll for jump
		movement.jumpPressed	= input.poll(Action::JUMP);
		// poll for crouch
		movement.crouchPressed	= input.poll(Action::CROUCH);
		// poll for sneak
		movement.isSneaking		= input.poll(Action::SNEAK);
	}

	if (movement.moveType == MovementType::NOCLIP) {
		if (input.poll(Action::JUMP))  movement.upMove += 1.0f;
        if (input.poll(Action::SNEAK)) movement.upMove -= 1.0f;
	}

	// maybe better to not expose directly and make a function to supply all movement inputs? or add to update();?
	movement.forwardView = camera->getForwardDirection();
	movement.rightView = camera->getRightDirection();

	movement.update(dt, map);

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