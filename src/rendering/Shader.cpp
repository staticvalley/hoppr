#include <Shader.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <Log.hpp>

// handles basic shader operations
Shader::Shader(const char* vertPath, const char* fragPath)
    : vertexPath(vertPath),
      fragmentPath(fragPath)
{
    id = buildProgram();

    if (id == 0) {
        Log::error(Log::Channel::RENDERER, "shader ('{}', '{}') failed to build. objects supplied with this shader will not draw", vertPath, fragPath);
    }

}

bool Shader::reload() {

    GLuint newId = buildProgram();

    if (newId == 0) {
        Log::warn(Log::Channel::RENDERER, "shader ('{}', '{}') reload failed, keeping previous version", vertexPath, fragmentPath);
        return false;
    }

    glDeleteProgram(id);
    id = newId;

    // reset cache for new build
    uniformCache.clear();

    Log::info(Log::Channel::RENDERER, "shader ('{}', '{}') reloaded successfully", vertexPath, fragmentPath);

    return true;
}

GLuint Shader::buildProgram() {

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentPath);

    // compile stage failed
    if (vertexShader == 0 || fragmentShader == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // delete shaders after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // check for linking errors and report
    GLint linkingSuccessful;
    glGetProgramiv(id, GL_LINK_STATUS, &linkingSuccessful);
    if (linkingSuccessful == GL_FALSE) {
        int errorLength;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &errorLength);
        std::string errorMessage(errorLength, '\0');
        glGetProgramInfoLog(id, errorLength, &errorLength, errorMessage.data());
        Log::error(Log::Channel::RENDERER, "shader ('{}', '{}') link error: {}", vertexShader, fragmentShader, errorMessage);
        glDeleteProgram(id);
        return 0;
    }

    return shaderProgram;
}

std::string Shader::readShaderFile(const char* path) {
    
    // open file and error check
    std::ifstream shaderFile(path);
    if (!shaderFile.is_open()) {
        Log::error(Log::Channel::RENDERER, "error opening shader file at \"{}\"", path);
        return "";
    }

    // read in entire file to string
    std::stringstream stringStream;
    stringStream << shaderFile.rdbuf();
    
    return stringStream.str();
}

GLuint Shader::compileShader(GLuint type, const char* path) {

    std::string source = readShaderFile(path);

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
        // this is so verbose to write out #openglblows
        int errorLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &errorLength);
        std::string errorMessage(errorLength, '\0');
        glGetShaderInfoLog(shaderID, errorLength, &errorLength, errorMessage.data());
        Log::error(Log::Channel::RENDERER, "shader compile error in '{}': {}", path, errorMessage);
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

void Shader::setUniformBool1(const char* name, bool value) {
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

    if (location == -1)
        Log::warn(Log::Channel::RENDERER, "uniform '{}' not found in shader ('{}', '{}')",
            name,
            vertexPath,
            fragmentPath
        );

    // write to cache
    uniformCache[name] = location;

    // return location
    return location;
}