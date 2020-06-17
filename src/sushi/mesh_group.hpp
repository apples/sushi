#ifndef SUSHI_MESH_GROUP_HPP
#define SUSHI_MESH_GROUP_HPP

#include "gl.hpp"
#include "common.hpp"
#include "iqm.hpp"

#include <string>
#include <memory>
#include <vector>

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

struct mesh_group {
    struct mesh {
        std::string name;
        int num_tris = 0;
        unique_buffer tris;
        unique_vertex_array vao;
    };

    unique_buffer position_buffer;
    unique_buffer texcoord_buffer;
    unique_buffer normal_buffer;
    unique_buffer tangent_buffer;
    unique_buffer blendindices_buffer;
    unique_buffer blendweights_buffer;
    unique_buffer color_buffer;
    std::vector<mesh> meshes;
};

auto load_meshes(const iqm::iqm_data& data) -> mesh_group;

/// Draws a mesh.
/// \param mesh The mesh to draw.
void draw_mesh(const mesh_group& group);

} // namespace sushi

#endif // SUSHI_MESH_GROUP_HPP