#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

class Camera {
public:

    Camera(glm::vec3 startPosition, GLfloat fov, GLfloat aspectRatio);
    ~Camera();
   
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();

    glm::vec3 getPosition();
    glm::vec3 getForwardDirection();
    glm::vec3 getRightDirection();
    glm::vec3 getUpDirection();

    void updateRoll(float z);
    void updatePosition(glm::vec3 newPosition);
    void updateAspectRatio(GLfloat height, GLfloat width);
    void updateViewXRelYRel(float x, float y);

private:

    // current camera position
    glm::vec3 position;

    // camera relative front
    glm::vec3 forward;

    // camera relative right
    glm::vec3 right;

    // camera relative up
    glm::vec3 up;

    // field of view
    GLfloat fov;
    
    // right/left view
    GLfloat yaw;

    // up/down view
    GLfloat pitch;

    // tilt view
    GLfloat roll;

    // changable by window resize
    GLfloat aspectRatio;

    GLfloat mouseSensitivity;

    // calculate camera relative front and up vectors
    void calcuateRelativeVectors();
};