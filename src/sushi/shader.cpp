//
// Created by Jeramy on 7/22/2015.
//

#include "shader.hpp"

#include "attrib_location.hpp"
#include "common.hpp"
#include "gl.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>

/// Sushi
namespace sushi {

unique_shader compile_shader(shader_type type, std::vector<const GLchar*> code) {
    unique_shader rv = make_unique_shader(GLenum(type));

    glShaderSource(rv.get(), code.size(), &code[0], nullptr);

    glCompileShader(rv.get());

    GLint log_length;
    glGetShaderiv(rv.get(), GL_INFO_LOG_LENGTH, &log_length);

    if (log_length > 0) {
        auto log = std::make_unique<GLchar[]>(log_length);
        glGetShaderInfoLog(rv.get(), log_length, nullptr, log.get());

        std::clog << "Shader compilation log:\n" << log.get() << std::endl;
    }

    GLint result;
    glGetShaderiv(rv.get(), GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        std::ostringstream oss;

        oss << "Shader compilation failed!" << std::endl;

        throw shader_error(oss.str());
    }

    return rv;
}

unique_shader compile_shader_file(shader_type type, const std::string& fname) {
    auto lines = load_file(fname);

    std::vector<const GLchar *> line_pointers;
    line_pointers.reserve(lines.size());
    std::transform(begin(lines), end(lines), std::back_inserter(line_pointers), [](auto &line) { return line.data(); });

    try {
        return compile_shader(type, line_pointers);
    } catch (const shader_error& e) {
        throw shader_error(fname + ": " + e.what());
    }
}

unique_program link_program(const std::vector<unique_shader>& shaders) {
    unique_program rv = make_unique_program();

    for (const auto& shader : shaders) {
        glAttachShader(rv.get(), shader.get());
    }

    for (const auto& [loc, name] : attrib_names) {
        glBindAttribLocation(rv.get(), static_cast<GLuint>(loc), name);
    }

    glLinkProgram(rv.get());

    GLint log_length;
    glGetProgramiv(rv.get(), GL_INFO_LOG_LENGTH, &log_length);

    if (log_length > 0) {
        auto log = std::make_unique<GLchar[]>(log_length);
        glGetProgramInfoLog(rv.get(), log_length, nullptr, log.get());

        std::clog << "Program compilation log:\n" << log.get() << std::endl;
    }

    GLint result;
    glGetProgramiv(rv.get(), GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        std::ostringstream oss;

        oss << "Program compilation failed!" << std::endl;

        throw std::runtime_error(oss.str());
    }

    return rv;
}


shader_base::shader_base(std::initializer_list<std::pair<sushi::shader_type, std::string>> sources) {
    auto compiled = std::vector<unique_shader>();
    compiled.reserve(sources.size());
    for (auto& source : sources) {
        compiled.push_back(compile_shader_file(source.first, source.second));
    }
    program = link_program(compiled);
}

void shader_base::bind() {
    set_program(program);
}

GLint shader_base::get_uniform_location(const std::string &name) const {
    return glGetUniformLocation(program.get(), name.data());
}

const unique_program &shader_base::get_program() const {
    return program;
}

}
