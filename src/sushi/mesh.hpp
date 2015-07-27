//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_MESH_HPP
#define SUSHI_MESH_HPP

#include "gl.hpp"
#include "common.hpp"

#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

/// Sushi
namespace sushi {

/// Deleter for OpenGL buffer objects.
struct buffer_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteBuffers(1, &buf);
    }
};

/// Deleter for OpenGL vertex array objects.
struct vertex_array_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteVertexArrays(1, &buf);
    }
};

/// A unique handle to an OpenGL buffer object.
using unique_buffer = unique_gl_resource<buffer_deleter>;

/// A unique handle to an OpenGL vertex array object.
using unique_vertex_array = unique_gl_resource<vertex_array_deleter>;

/// Creates a unique OpenGL buffer object.
/// \return A unique buffer object.
inline unique_buffer make_unique_buffer() {
    GLuint buf;
    glGenBuffers(1, &buf);
    return unique_buffer(buf);
}

/// Creates a unique OpenGL vertex array object.
/// \return A unique vertex array object.
inline unique_vertex_array make_unique_vertex_array() {
    GLuint buf;
    glGenVertexArrays(1, &buf);
    return unique_vertex_array(buf);
}

/// A static OpenGL mesh made of triangles.
struct static_mesh {
    unique_vertex_array vao;
    unique_buffer vertex_buffer;
    int num_triangles = 0;
};

struct attrib_location {
    static constexpr auto POSITION = 0;
    static constexpr auto TEXCOORD = 1;
    static constexpr auto NORMAL = 2;
};

/// Loads a mesh from an OBJ file.
/// The following OBJ directives are supported:
/// - `#` - Comments
/// - `v` - Vertex location.
/// - `vn` - Vertex normal.
/// - `vt` - Vertex texture coordinate.
/// - `f` - Face (triangles only).
/// \param fname File name.
/// \return The static mesh described by the file.
static_mesh load_static_mesh(const std::string& fname) {
    std::ifstream file(fname);
    std::string line;
    std::string word;
    int line_number = 0;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::vector<GLfloat> data;
    int num_tris = 0;

    while (getline(file, line)) {
        ++line_number;
        std::istringstream iss(line);
        iss >> word;

        if (word == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (word == "vn") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            normals.push_back(v);
        } else if (word == "vt") {
            glm::vec2 v;
            iss >> v.x >> v.y;
            v.y = 1.0 - v.y;
            texcoords.push_back(v);
        } else if (word == "f") {
            int index;
            for (int i = 0; i < 3; ++i) {
                iss >> word;
                std::replace(begin(word), end(word), '/', ' ');
                std::istringstream iss2(word);
                iss2 >> index;
                --index;
                data.push_back(vertices[index].x);
                data.push_back(vertices[index].y);
                data.push_back(vertices[index].z);
                iss2 >> index;
                --index;
                data.push_back(texcoords[index].x);
                data.push_back(texcoords[index].y);
                iss2 >> index;
                --index;
                data.push_back(normals[index].x);
                data.push_back(normals[index].y);
                data.push_back(normals[index].z);
            }
            ++num_tris;
        } else if (word[0] == '#') {
            // pass
        } else {
            std::clog << "sushi::load_static_mesh(): Warning: Unknown OBJ directive at " << fname << "[" << line_number
            << "]: \"" << word << "\"." << std::endl;
        }
    }

    static_mesh rv;

    rv.vao = make_unique_vertex_array();
    rv.vertex_buffer = make_unique_buffer();
    rv.num_triangles = num_tris;

    glBindBuffer(GL_ARRAY_BUFFER, rv.vertex_buffer.get());
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

    glBindVertexArray(rv.vao.get());

    auto stride = sizeof(GLfloat) * (3 + 2 + 3);
    glEnableVertexAttribArray(attrib_location::POSITION);
    glEnableVertexAttribArray(attrib_location::TEXCOORD);
    glEnableVertexAttribArray(attrib_location::NORMAL);
    glVertexAttribPointer(attrib_location::POSITION, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const GLvoid*>(0));
    glVertexAttribPointer(attrib_location::TEXCOORD, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const GLvoid*>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib_location::NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const GLvoid*>(sizeof(GLfloat) * (3 + 2)));

    return rv;
}

/// Draws a mesh.
/// \param mesh The mesh to draw.
inline void draw_mesh(const static_mesh& mesh) {
    glBindVertexArray(mesh.vao.get());
    glDrawArrays(GL_TRIANGLES, 0, mesh.num_triangles * 3);
}

}

#endif //SUSHI_MESH_HPP
