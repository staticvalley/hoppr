#pragma once

#include <Shader.hpp>
#include <Camera.hpp>
#include <glad/glad.h>
#include <string>

#define SKYBOX_TYPE_OPENGL 0    // skybox faces are oriented in opengl format
#define SKYBOX_TYPE_GOLDSRC 1   // skybox faces are oriented in goldsrc format

#define SKYBOX_GOLDSRC_LF 0     // goldsrc left sklybox face
#define SKYBOX_GOLDSRC_RT 1     // goldsrc right sklybox face
#define SKYBOX_GOLDSRC_UP 2     // goldsrc up sklybox face
#define SKYBOX_GOLDSRC_DN 3     // goldsrc down sklybox face
#define SKYBOX_GOLDSRC_FT 4     // goldsrc front sklybox face
#define SKYBOX_GOLDSRC_BK 5     // goldsrc back sklybox face

class Skybox {
public:

    /**
     * @brief default constructor, ideally shouldnt be used to initialize object
     */
    Skybox() : vao(0), vbo(0), textureId(0), shader(nullptr) {}

    /**
     * @brief 
     * @param faceFilePaths full paths to each skybox face
     * @param skyboxShader shader to bind when drawing skybox
     * @param skyboxType input sky texture format, needed for orientation (`SKYBOX_TYPE_OPENGL`, `SKYBOX_TYPE_GOLDSRC`)
     */
    Skybox(const std::string faceFilePaths[], Shader* skyboxShader, uint8_t skyboxType);
    ~Skybox();

    // create move constructor
    Skybox(Skybox&& other) noexcept; // create move constructor
    Skybox& operator=(Skybox&& other) noexcept; // create move assignment

    // disallow copy
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void draw(Camera& camera);

private:
    GLuint vao, vbo;
    GLuint textureId;
    Shader* shader;

    /**
     * @brief loads skybox texture files into 2D sampler cubemap object
     * @param faceFilePaths full paths to each skybox face
     * @param skyboxType input sky texture format, needed for orientation (`SKYBOX_TYPE_OPENGL`, `SKYBOX_TYPE_GOLDSRC`)
     * @return opengl texture id
     */
    GLuint loadCubeMap(const std::string faceFilePaths[], uint8_t skyboxType);

    /**
     * @brief flips image data along the x axis
     * @param data raw pixel bytes
     * @param width width of image
     * @param height height of image
     * @param format representation of image data (`GL_RGB` or `GL_RGBA`)
     */
    void flipHorizontal(unsigned char* data, int width, int height, GLenum format);

    /**
     * @brief rotates image data 90 degrees clockwise `steps` times
     * @param data raw pixel bytes
     * @param width width of image
     * @param height height of image
     * @param format representation of image data (`GL_RGB` or `GL_RGBA`)
     * @param steps how many rotations should be performed (ie. 2 = 180 rotation clockwise)
     */
    void rotate90(unsigned char* data, int width, int height, GLenum format, int steps);

};