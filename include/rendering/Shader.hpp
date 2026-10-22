#pragma once

#include <format>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <unordered_map>


// handles basic shader operations
class Shader {
public:


    /**
     * @brief constructs shader from files
     * @param vertPath full path to vertex shader file
     * @param fragPath full path to fragment shader file
     */
    Shader(const char* nameTag, const char* vertPath, const char* fragPath);
    ~Shader();

    /**
     * @brief recompile shader programs from sourcefiles
     * @return reload status (true == reloaded, false == rollback)
     */
    bool reload();

    /**
     * @brief bind shader as active program 
     */
    void bind();

    /**
     * @brief unbinds shader as active program
     */
    void unbind();
    
    /**
     * @brief set bool uniform on shader
     * @param name uniform identifier
     * @param value uniform boolean value
     */
    void setUniformBool1(const char* name, bool value);

    /**
     * @brief set int uniform on shader
     * @param name uniform identifier
     * @param value uniform int value
     */
    void setUniformInt1(const char* name, int value);

    /**
     * @brief set float uniform on shader
     * @param name uniform identifier
     * @param value uniform float value
     */
    void setUniformFloat1(const char* name, float value);

    /**
     * @brief set 3 value vector uniform on shader
     * @param name uniform identifier
     * @param value uniform glm::vec3 value
     */
    void setUniformVec3(const char* name, const glm::vec3& value);

    /**
    * @brief set 4 value vector uniform on shader
    * @param name uniform identifier
    * @param value uniform glm::vec4 value
    */
    void setUniformVec4(const char* name, const glm::vec4& value);

    /**
     * @brief set 4x4 matrix uniform on shader
     * @param name uniform identifier
     * @param value uniform glm::mat4 value
     */
    void setUniformMat4(const char* name, const glm::mat4& value);

    std::string debugShaderString() const {
        return std::format(
            "Shader: {{\n"
            "  .nameTag = {},\n"
            "  .vertPath = {},\n"
            "  .fragPath = {},\n"
            "  .id = {},\n"
            "}}",
            nameTag,
            vertexPath,
            fragmentPath,
            id == 0 ? "uncompiled" : std::format("compiled (id: {})", id));
    }

private:

    /// name of shader (for debug purposes)
    std::string nameTag;

    /// shader id
    GLuint id = 0;

    /// path to vertex shader source code
    const char* vertexPath    = nullptr;

    /// path to fragment shader source code
    const char* fragmentPath  = nullptr;

    /// shader uniform name cache (name -> location)
    std::unordered_map<std::string, GLint> uniformCache;
    
    /**
     * @brief check shader for uniform name location
     * @param name name of uniform (ie. "u_model")
     * @return location id of uniform
     */
    GLint getUniformLocation(const char* name);

    /**
     * @brief builds shader program given source files
     * @return shader program id (0 == failed compile)
     */
    GLuint buildProgram();

    /**
     * @brief read shader file to a string
     * @param path path to shader source code
     * @return source code in string
     */
    std::string readShaderFile(const char* path);

    /**
     * @brief compile vertex/fragment shader given source code
     * @param type type of shader (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER)
     * @param path path to shader source code
     * @return shader id
     */
    GLuint compileShader(GLuint type, const char* path);

    /**
     * @brief check if shader is correctly compiled
     * @return status of compilation (true == compiled, false == uncompiled)
     */
    bool isCompiled() const { return id != 0; }
};

// print formatter for Shaders
template <>
struct std::formatter<Shader> : std::formatter<char> {
    auto format(const Shader& shader, std::format_context& ctx) const {
        return std::format_to(ctx.out(),"{}", shader.debugShaderString());
    }
};
