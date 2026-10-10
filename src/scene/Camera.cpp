#pragma once

#include <Camera.hpp>
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>


Camera::Camera(glm::vec3 startPosition, GLfloat fov, GLfloat aspectRatio)
	: position(startPosition),
	  fov(fov),
	  aspectRatio(aspectRatio),
	  // view presets
	  yaw(-90.0f),
	  pitch(0.0f),
	  roll(0.0f),
	  // control presets
	  mouseSensitivity(0.03f),
	  // basic starting camera relative vectors
	  forward(0.0f, 0.0f, -1.0f),
	  up(0.0f, 1.0f, 0.0f),
	  right(1.0f, 0.0f, 0.0f)
{
	calcuateRelativeVectors();
}

Camera::~Camera() {}

glm::mat4 Camera::getViewMatrix() {
	// lookat args (camera position, target position, up)
	return glm::lookAt(position, position + forward, up);
}

glm::mat4 Camera::getProjectionMatrix() {
	/*
	 * glm::perspective args
	 * - fov
	 * - aspect ratio
	 * - near frustum cutoff
	 * - far frustum cutoff
	 */
	return glm::perspective(
		glm::radians(fov),
		aspectRatio,
		1.0f,
		100000.0f
	);
}

glm::vec3 Camera::getPosition() {
	return position;
}

glm::vec3 Camera::getForwardDirection() {
	return forward;
}

glm::vec3 Camera::getRightDirection() {
	return right;
}

glm::vec3 Camera::getUpDirection() {
	return up;
}

void Camera::updatePosition(glm::vec3 newPosition) {
	position = newPosition;
};

void Camera::updateAspectRatio(GLfloat height, GLfloat width) {
	aspectRatio = height / width;
};

void Camera::updateViewXRelYRel(float x, float y) {

	// sdl3's xrel event value is positive when moving right, right is negative in world space, so negation is needed
	yaw += x * mouseSensitivity;
	pitch -= y * mouseSensitivity;

	// clamp pitch so you cant roll over top
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	// recalculate forward/right/up vectors now that view has changed
	calcuateRelativeVectors();
}

void Camera::updateRoll(float z) {
	roll = z;
}

void Camera::calcuateRelativeVectors() {
	/*
	 * forward vector
	 * eulers angles calculation
	 */
	forward = glm::normalize(
		glm::vec3(
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
			sin(glm::radians(pitch)),
			sin(glm::radians(yaw)) * cos(glm::radians(pitch))
		)
	);

	// base right vector (cross product of camera forward and world up (0, 1, 0))
	glm::vec3 baseRight = glm::normalize(
		glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))
	);

	// up vector (cross product of camera forward and camera right)
	glm::vec3 baseUp = glm::normalize(
		glm::cross(baseRight, forward)
	);

	glm::mat4 rollMatrix = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(roll),
		forward
	);

	right = glm::normalize(glm::vec3(rollMatrix * glm::vec4(baseRight, 0.0f)));
	up    = glm::normalize(glm::vec3(rollMatrix * glm::vec4(baseUp, 0.0f)));
}