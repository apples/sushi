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
#include <variant>
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

enum class attrib_location : GLuint {
    POSITION = 0,
    TEXCOORD = 1,
    NORMAL = 2,
    TANGENT = 3,
    BLENDINDICES = 4,
    BLENDWEIGHTS = 5,
    COLOR = 6,
};

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

    struct transform {
        glm::vec3 pos;
        glm::quat rot;
        glm::vec3 scl;
    };

    std::vector<int> bone_parents;
    std::vector<glm::mat4> bone_mats;
    std::vector<glm::mat4> bone_mats_inverse;
    std::vector<transform> frame_transforms;
    std::vector<animation> animations;
};

auto load_meshes(const iqm::iqm_data& data) -> mesh_group;

auto load_skeleton(const iqm::iqm_data& data) -> skeleton;

auto get_animation_index(const skeleton& skele, const std::string& name) -> std::optional<int>;

auto get_frame(const skeleton& skele, const skeleton::animation& anim, float time)
    -> std::span<const skeleton::transform>;

auto blend_frames(
    const skeleton& skele,
    std::span<const skeleton::transform> from,
    std::span<const skeleton::transform> to,
    float alpha) -> std::vector<glm::mat4>;

/// Draws a mesh.
/// \param mesh The mesh to draw.
void draw_mesh(const mesh_group& group, const skeleton* skele, std::optional<int> anim_index, float time, bool smooth);

/// Draws a mesh.
/// \param mesh The mesh to draw.
void draw_mesh(const mesh_group& group);

} // namespace sushi

#endif //SUSHI_MESH_HPP
