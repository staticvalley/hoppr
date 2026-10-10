#pragma once

#include <Skybox.hpp>
#include <Shader.hpp>
#include <Camera.hpp>
#include <print>

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include "../libs/stb/stb_image.h"
#include <string>
#include <cstring>

#include <Log.hpp>

// define unit cube for skybox
static const GLfloat SKYBOX_VERTICES[] = {
    -1, 1,-1,  -1,-1,-1,   1,-1,-1,   1,-1,-1,   1, 1,-1,  -1, 1,-1,
    -1,-1, 1,  -1,-1,-1,  -1, 1,-1,  -1, 1,-1,  -1, 1, 1,  -1,-1, 1,
    1,-1,-1,   1,-1, 1,   1, 1, 1,   1, 1, 1,   1, 1,-1,   1,-1,-1,
    -1,-1, 1,  -1, 1, 1,   1, 1, 1,   1, 1, 1,   1,-1, 1,  -1,-1, 1,
    -1, 1,-1,   1, 1,-1,   1, 1, 1,   1, 1, 1,  -1, 1, 1,  -1, 1,-1,
    -1,-1,-1,  -1,-1, 1,   1,-1, 1,   1,-1, 1,   1,-1,-1,  -1,-1,-1
};

Skybox::Skybox(const std::string faceFilePaths[], Shader* skyboxShader, uint8_t skyboxType) 
    : shader(skyboxShader)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_VERTICES), SKYBOX_VERTICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    textureId = loadCubeMap(faceFilePaths, skyboxType);
}


GLuint Skybox::loadCubeMap(const std::string faceFilePaths[], uint8_t skyboxType) {

    // create and bind texture object as cube map
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    int width, height, nChannels;
    for (int i = 0; i < 6; i++) {

        bool flipHorizontally = false;

        // see line 66
        if (skyboxType == SKYBOX_TYPE_GOLDSRC) {
            // top and bottom face textures need to be flipped vertically for opengl cubemap
        	stbi_set_flip_vertically_on_load(i == SKYBOX_GOLDSRC_UP || i == SKYBOX_GOLDSRC_DN);
        }

    	unsigned char* data = stbi_load(faceFilePaths[i].c_str(), &width, &height, &nChannels, 0);
        if (!data) {
            Log::warn(Log::Channel::RENDERER, "Error loading skybox face {}: {}", i, faceFilePaths[i]);
            return id;
        }

        GLenum format = nChannels == 4 ? GL_RGBA : GL_RGB;

        /**
        * goldsrc tga skyboxes have different orientations
        * - up face must be rotated 90 degrees clockwise
        * - down face must be rotated 90 degrees counter-clockwise
        * - front, back, left, and right faces must be horizontally flipped
        */
        if(skyboxType == SKYBOX_TYPE_GOLDSRC) {
            switch (i) {
            case SKYBOX_GOLDSRC_UP:
                rotate90(data, width, height, format, 1);
                break;
            case SKYBOX_GOLDSRC_DN:
                rotate90(data, width, height, format, 3);
                break;
            default:
                flipHorizontal(data, width, height, format);
                break;
            }
        }
            
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, 
            format, 
            width, 
            height, 
            0, 
            format,
            GL_UNSIGNED_BYTE, 
            data
        );

        stbi_image_free(data);
    }

    // reset
    stbi_set_flip_vertically_on_load(false);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return id;
}

void Skybox::flipHorizontal(unsigned char* data, int width, int height, GLenum format) {
	int channels = format == GL_RGB ? 3 : 4;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width / 2; x++) {
            int leftIndex = (y * width + x) * channels;
            int rightIndex = (y * width + (width - 1 - x)) * channels;
            for(int c = 0; c < channels; c++)
                std::swap(data[leftIndex + c], data[rightIndex + c]);
        }
	}
}

void Skybox::rotate90(unsigned char* data, int width, int height, GLenum format, int steps) {
    // normalize to 0-3, handle negatives
    steps = ((steps % 4) + 4) % 4; 
    int channels = format == GL_RGBA ? 4 : 3;
    std::vector<unsigned char> tmp(width * height * channels);
    for (int s = 0; s < steps; s++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int src = (y * width + x) * channels;
                int dst = (x * height + (height - 1 - y)) * channels;
                for (int c = 0; c < channels; c++)
                    tmp[dst + c] = data[src + c];
            }
        }
        std::memcpy(data, tmp.data(), tmp.size());
    }
}

Skybox::~Skybox() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &textureId);
    delete shader;
}

void Skybox::draw(Camera& camera) {
    
    if (!shader) {
	    Log::error(Log::Channel::RENDERER, "Error loading skybox shader: shader is null or not assigned. Skipping draw.");
        return;
    }

    // bind skybox specific shader
    shader->bind();

    // clip translation from view matrix so skybox is always at origin
    glm::mat4 view = glm::mat4(
        glm::mat3(camera.getViewMatrix())
    );

    shader->setUniformMat4("u_view", view);
    shader->setUniformMat4("u_projection", camera.getProjectionMatrix());

    // skybox texture at slot 0
    shader->setUniformInt1("u_skybox", 0);
    
    // draw box with different depth drawing function
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

Skybox::Skybox(Skybox&& other) noexcept
    : shader(other.shader), vao(other.vao), vbo(other.vbo), textureId(other.textureId) {
    
    // null out other
    other.shader = nullptr;
    other.vao = 0;
    other.vbo = 0;
    other.textureId = 0;
}

Skybox& Skybox::operator=(Skybox&& other) noexcept {
    if (this != &other) {
        // clean up existing resources
        delete shader;
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteTextures(1, &textureId);
        
        // take ownership
        shader = other.shader;
        vao = other.vao;
        vbo = other.vbo;
        textureId = other.textureId;
        
        // null out other
        other.shader = nullptr;
        other.vao = 0;
        other.vbo = 0;
        other.textureId = 0;
    }
    return *this;
}