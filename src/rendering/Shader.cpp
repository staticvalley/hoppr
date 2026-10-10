#pragma once

#include <Shader.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <Log.hpp>

// handles basic shader operations
Shader::Shader(const char* vertPath, const char* fragPath) {
    
    // create shader
    id = glCreateProgram();

    // compile vertex and fragment shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, parseShaderFile(vertPath));
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, parseShaderFile(fragPath));

    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    glLinkProgram(id);
    
    // check for linking errors and report
    GLint errorCode;
    glGetProgramiv(id, GL_LINK_STATUS, &errorCode);
    if (errorCode == GL_FALSE) {
        int errorLength;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &errorLength);
        std::string errorMessage(errorLength, '\0');
        glGetProgramInfoLog(id, errorLength, &errorLength, errorMessage.data());
        Log::error(Log::Channel::RENDERER, "Shader link error: {}", errorMessage);
    }

    // delete shaders now that they are linked
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

std::string Shader::parseShaderFile(const char* filePath) {
    
    // open file and error check
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        Log::error(Log::Channel::RENDERER, "error opening shader file at \"{}\"", filePath);
        return "";
    }

    // read in entire file to string using std::stringstream
    std::stringstream stringStream;
    stringStream << shaderFile.rdbuf();
    
    return stringStream.str();
}

GLuint Shader::compileShader(GLuint type, const std::string& source) {
    
    // if shader code string does not exist, pass compiling
    if (source.empty()) return 0;

    const char* srcStr = source.c_str();

    // create and compile shader
    GLuint shaderID = glCreateShader(type);
    glShaderSource(shaderID, 1, &srcStr, nullptr);
    glCompileShader(shaderID);

    // check for compiling errors and report
    GLint errorCode;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &errorCode);
    if (errorCode == GL_FALSE) {
        // get error message and print
        int errorLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &errorLength);
        std::string errorMessage(errorLength, '\0');
        glGetShaderInfoLog(shaderID, errorLength, &errorLength, errorMessage.data());
        Log::error(Log::Channel::RENDERER, "Shader compile error: {}", errorMessage);
        return 0;
    }

    return shaderID;
}

Shader::~Shader() {
    glDeleteProgram(id);
}

void Shader::bind() {
    glUseProgram(id);
}

void Shader::unbind() {
    glUseProgram(0);
}

void Shader::setUniformBool1(const char* name, const bool value) {
    glUniform1i(getUniformLocation(name), (int)value);
}

void Shader::setUniformInt1(const char* name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniformFloat1(const char* name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniformVec4(const char* name, const glm::vec4& value) {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setUniformVec3(const char* name, const glm::vec3& value) {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setUniformMat4(const char* name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

GLint Shader::getUniformLocation(const char* name) {
    
    // check cache for uniform location first
    if (uniformCache.contains(name))
        return uniformCache[name];

    // otherwise, search for location
    GLint location = glGetUniformLocation(id, name);
    
    // write to cache
    uniformCache[name] = location;

    // return location
    return location;
}