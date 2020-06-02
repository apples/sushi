//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_MESH_HPP
#define SUSHI_MESH_HPP

#include "gl.hpp"
#include "common.hpp"
#include "iqm.hpp"

#include <string>
#include <memory>
#include <map>
#include <optional>
#include <span>
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

/// A static OpenGL mesh made of triangles.
struct static_mesh {
    unique_vertex_array vao;
    unique_buffer vertex_buffer;
    int num_triangles = 0;
    float bounding_sphere = 0;
};

enum attrib_location : GLuint {
    POSITION = 0,
    TEXCOORD = 1,
    NORMAL = 2,
    TANGENT = 3,
    BLENDINDICES = 4,
    BLENDWEIGHTS = 5,
    COLOR = 6,
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
static_mesh load_static_mesh_file(const std::string &fname);

struct Tri {
    using Index3 = std::vector<glm::vec3>::size_type;
    using Index2 = std::vector<glm::vec2>::size_type;
    struct Vert {
        Index3 pos;
        Index3 norm;
        Index2 tex;
    };
    Vert verts[3];
};

static_mesh load_static_mesh_data(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texcoords, const std::vector<Tri>& tris);

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

struct skeleton {
    struct animation {
        std::string name;
        int first_frame;
        int num_frames;
        float framerate;
        bool loop;
    };

    std::vector<int> bone_parents;
    std::vector<glm::mat4> bone_mats;
    std::vector<glm::mat4> frame_mats;
    std::vector<animation> animations;
};

auto load_meshes(const iqm::iqm_data& data) -> mesh_group;

auto load_skeleton(const iqm::iqm_data& data) -> skeleton;

auto get_animation_index(const skeleton& skele, const std::string& name) -> std::optional<int>;

auto get_frame(const skeleton& skele, int anim_index, float time) -> std::span<const glm::mat4>;

auto blend_frames(
    const skeleton& skele, int anim_index, std::span<const glm::mat4> from, std::span<const glm::mat4> to, float time)
    -> std::vector<glm::mat4>;

/// Draws a mesh.
/// \param mesh The mesh to draw.
inline void draw_mesh(const static_mesh& mesh) {
    glBindVertexArray(mesh.vao.get());
    SUSHI_DEFER { glBindVertexArray(0); };
    glDrawArrays(GL_TRIANGLES, 0, mesh.num_triangles * 3);
}

/// Draws a mesh.
/// \param mesh The mesh to draw.
inline void draw_mesh(const mesh_group& group, const skeleton* skele, std::optional<int> anim_index, float time) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto animated_uniform = glGetUniformLocation(program, "Animated");

    if (skele) {
        auto frame_mats = [&] {
            if (anim_index) {
                auto mats = get_frame(*skele, *anim_index, time);
                return blend_frames(*skele, *anim_index, mats, mats, time);
            } else {
                return skele->bone_mats;
            }
        }();

        auto bones_uniform = glGetUniformLocation(program, "Bones");

        glUniform1i(animated_uniform, 1);
        glUniformMatrix4fv(bones_uniform, frame_mats.size(), GL_FALSE, glm::value_ptr(frame_mats[0]));
    } else {
        glUniform1i(animated_uniform, 0);
    }

    SUSHI_DEFER { glBindVertexArray(0); };

    for (const auto& mesh : group.meshes) {
        glBindVertexArray(mesh.vao.get());
        glDrawElements(GL_TRIANGLES, mesh.num_tris * 3, GL_UNSIGNED_INT, nullptr);
    }
}

} // namespace sushi

#endif //SUSHI_MESH_HPP
