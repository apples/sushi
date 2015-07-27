//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_SHADER_HPP
#define SUSHI_SHADER_HPP

#include "common.hpp"

#include "gl.hpp"

#include <stdexcept>
#include <string>
#include <vector>

/// Sushi
namespace sushi {

/// Shader types.
enum class shader_type : GLenum {
    VERTEX = GL_VERTEX_SHADER,
    FRAGMENT = GL_FRAGMENT_SHADER
};

/// Deleter for OpenGL shader objects.
struct shader_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        GLuint shader = p;
        glDeleteShader(shader);
    }
};

/// Deleter for OpenGL shader program objects.
struct program_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        GLuint program = p;
        glDeleteProgram(program);
    }
};

/// A unique handle to an OpenGL shader object.
using unique_shader = unique_gl_resource<shader_deleter>;

/// A unique handle to an OpenGL shader program object.
using unique_program = unique_gl_resource<program_deleter>;

/// Creates a unique OpenGL shader object.
/// \return A unique shader object.
inline unique_shader make_unique_shader(GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    if (shader == 0) {
        throw std::runtime_error("Failed to create shader!");
    }
    return unique_shader(shader);
}

/// Creates a unique OpenGL shader program object.
/// \return A unique shader program object.
inline unique_program make_unique_program() {
    GLuint program = glCreateProgram();
    if (program == 0) {
        throw std::runtime_error("Failed to create shader program!");
    }
    return unique_program(program);
}

/// Loads and compiles an OpenGL shader from a file.
/// \param type The type of shader to be created.
/// \param fname File name.
/// \return A unique shader object.
unique_shader compile_shader_file(shader_type type, const std::string& fname);

/// Links a shader program.
/// \pre All of the shaders are compiled.
/// \param shaders List of shaders to link.
/// \return Unique handle to the new shader program.
unique_program link_program(const std::vector<const_reference_wrapper<unique_shader>>& shaders);

/// Sets the current shader program.
/// \pre The program was successfully linked.
/// \param program Shader program to set.
inline void set_program(const unique_program& program) {
    glUseProgram(program.get());
}

/// Sets a uniform in the shader program.
/// \param program The shader program.
/// \param name The name of the uniform.
/// \param data The value to set to the uniform.
template<typename T>
void set_uniform(const unique_program& program, const std::string& name, const T& data);

template<>
inline void set_uniform(const unique_program& program, const std::string& name, const glm::mat4& mat) {
    glProgramUniformMatrix4fv(program.get(), glGetUniformLocation(program.get(), name.data()), 1, GL_FALSE,
                              glm::value_ptr(mat));
}

template<>
inline void set_uniform(const unique_program& program, const std::string& name, const GLint& i) {
    glProgramUniform1i(program.get(), glGetUniformLocation(program.get(), name.data()), i);
}

}

#endif //SUSHI_SHADER_HPP
