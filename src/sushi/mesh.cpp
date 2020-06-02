//
// Created by Jeramy on 8/22/2015.
//

#include "mesh.hpp"

#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>

namespace sushi {

namespace {

static_mesh upload_static_mesh(const std::vector<GLfloat>& data, int num_tris) {
    static_mesh rv;

    rv.vao = make_unique_vertex_array();
    rv.vertex_buffer = make_unique_buffer();
    rv.num_triangles = num_tris;
    rv.bounding_sphere = 0;

    for (int i=0; i<num_tris*3; ++i) {
        auto pos = glm::vec3{data[i*8+0],data[i*8+1],data[i*8+2]};
        rv.bounding_sphere = std::max(glm::length(pos), rv.bounding_sphere);
    }

    glBindBuffer(GL_ARRAY_BUFFER, rv.vertex_buffer.get());
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

    glBindVertexArray(rv.vao.get());
    SUSHI_DEFER { glBindVertexArray(0); };

    auto stride = sizeof(GLfloat) * (3 + 2 + 3);
    glEnableVertexAttribArray(attrib_location::POSITION);
    glEnableVertexAttribArray(attrib_location::TEXCOORD);
    glEnableVertexAttribArray(attrib_location::NORMAL);
    glVertexAttribPointer(
        attrib_location::POSITION, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(0));
    glVertexAttribPointer(
        attrib_location::TEXCOORD, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(
        attrib_location::NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * (3 + 2)));

    return rv;
}

template <typename T>
auto load_buffer(sushi::attrib_location loc, const std::vector<T>& arr) -> unique_buffer {
    if (!arr.empty()) {
        auto buf = make_unique_buffer();
        glBindBuffer(GL_ARRAY_BUFFER, buf.get());
        glBufferData(GL_ARRAY_BUFFER, arr.size() * sizeof(arr[0]), &arr[0], GL_STATIC_DRAW);
        return buf;
    } else {
        return nullptr;
    }
};

void bind_attrib_float(
    sushi::attrib_location loc, const unique_buffer& buf, GLint size, std::size_t offset, std::array<float, 4> init) {

    glVertexAttrib4fv(loc, init.data());

    if (buf) {
        glEnableVertexAttribArray(loc);
        glBindBuffer(GL_ARRAY_BUFFER, buf.get());
        glVertexAttribPointer(
            loc, size, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const void*>(offset * size * sizeof(GLfloat)));
    }
};

void bind_attrib_ubyte(
    sushi::attrib_location loc,
    const unique_buffer& buf,
    GLint size,
    GLboolean normalize,
    std::size_t offset,
    std::array<std::uint8_t, 4> init) {

    glVertexAttrib4ubv(loc, init.data());

    if (buf) {
        glEnableVertexAttribArray(loc);
        glBindBuffer(GL_ARRAY_BUFFER, buf.get());
        glVertexAttribPointer(
            loc, size, GL_UNSIGNED_BYTE, normalize, 0, reinterpret_cast<const void*>(offset * size * sizeof(GLubyte)));
    }
};

} // static

static_mesh load_static_mesh_file(const std::string &fname) {
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
            std::clog << "sushi::load_static_mesh_file(): Warning: Unknown OBJ directive at " << fname << "[" <<
            line_number
            << "]: \"" << word << "\"." << std::endl;
        }
    }

    return upload_static_mesh(data, num_tris);
}

static_mesh load_static_mesh_data(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texcoords, const std::vector<Tri>& tris) {
    std::vector<GLfloat> data;
    int num_tris = tris.size();

    for (const auto& tri : tris) {
        for (const auto& vert : tri.verts) {
            data.push_back(positions[vert.pos].x);
            data.push_back(positions[vert.pos].y);
            data.push_back(positions[vert.pos].z);
            data.push_back(texcoords[vert.tex].x);
            data.push_back(texcoords[vert.tex].y);
            data.push_back(normals[vert.norm].x);
            data.push_back(normals[vert.norm].y);
            data.push_back(normals[vert.norm].z);
        }
    }

    return upload_static_mesh(data, num_tris);
}

auto load_meshes(const iqm::iqm_data& data) -> mesh_group {
    mesh_group group;

    group.position_buffer = load_buffer(attrib_location::POSITION, data.vertexarrays.position);
    group.texcoord_buffer = load_buffer(attrib_location::TEXCOORD, data.vertexarrays.texcoord);
    group.normal_buffer = load_buffer(attrib_location::NORMAL, data.vertexarrays.normal);
    group.tangent_buffer = load_buffer(attrib_location::TANGENT, data.vertexarrays.tangent);
    group.blendindices_buffer = load_buffer(attrib_location::BLENDINDICES, data.vertexarrays.blendindexes);
    group.blendweights_buffer = load_buffer(attrib_location::BLENDWEIGHTS, data.vertexarrays.blendweights);
    group.color_buffer = load_buffer(attrib_location::COLOR, data.vertexarrays.color);

    for (auto& iqm_mesh : data.meshes) {
        auto mesh = mesh_group::mesh{};
        mesh.name = iqm_mesh.name;
        mesh.num_tris = iqm_mesh.num_triangles;
        mesh.tris = make_unique_buffer();
        mesh.vao = make_unique_vertex_array();
        glBindVertexArray(mesh.vao.get());
        SUSHI_DEFER { glBindVertexArray(0); };

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.tris.get());
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            iqm_mesh.num_triangles * 3 * 4,
            &data.triangles[iqm_mesh.first_triangle],
            GL_STATIC_DRAW);

        bind_attrib_float(attrib_location::POSITION, group.position_buffer, 3, iqm_mesh.first_vertex, {});
        bind_attrib_float(attrib_location::TEXCOORD, group.texcoord_buffer, 2, iqm_mesh.first_vertex, {});
        bind_attrib_float(attrib_location::NORMAL, group.normal_buffer, 3, iqm_mesh.first_vertex, {});
        bind_attrib_float(attrib_location::TANGENT, group.tangent_buffer, 3, iqm_mesh.first_vertex, {});
        bind_attrib_ubyte(attrib_location::BLENDINDICES, group.blendindices_buffer, 4, GL_FALSE, iqm_mesh.first_vertex, {});
        bind_attrib_ubyte(attrib_location::BLENDWEIGHTS, group.blendweights_buffer, 4, GL_TRUE, iqm_mesh.first_vertex, {});
        bind_attrib_float(attrib_location::COLOR, group.color_buffer, 4, iqm_mesh.first_vertex, {1.f, 1.f, 1.f, 1.f});

        group.meshes.push_back(std::move(mesh));
    }

    return group;
}

auto load_skeleton(const iqm::iqm_data& data) -> skeleton {
    skeleton skele;

    // Bones

    for (const auto& iqm_joint : data.joints) {
        skele.bone_parents.push_back(iqm_joint.parent);

        auto mat = glm::mat4(1.f);
        mat = glm::translate(mat, iqm_joint.pos);
        mat = mat * glm::mat4_cast(glm::normalize(iqm_joint.rot));
        mat = glm::scale(mat, iqm_joint.scl);

        if (iqm_joint.parent >= 0) {
            mat = skele.bone_mats[iqm_joint.parent] * mat;
        }

        skele.bone_mats.push_back(mat);
    }

    // Animations

    for (const auto& iqm_anim : data.anims) {
        auto anim = skeleton::animation{};
        anim.name = iqm_anim.name;
        anim.first_frame = iqm_anim.first_frame;
        anim.num_frames = iqm_anim.num_frames;
        anim.framerate = iqm_anim.framerate;
        anim.loop = iqm_anim.loop;

        skele.animations.push_back(std::move(anim));
    }

    // Frames

    auto current_frame_channel = 0u;

    for (auto frame_index = 0u; frame_index < data.frames.size() / data.num_framechannels; ++frame_index) {
        for (auto joint_index = 0u; joint_index < data.poses.size(); ++joint_index) {
            const auto& pose = data.poses[joint_index];

            auto pose_pos = glm::vec3{};
            auto pose_rot = glm::quat{};
            auto pose_scl = glm::vec3{};

            auto assign_channel_value = [&](int channel, float value) {
                switch (channel) {
                    case 0: pose_pos.x = value; break;
                    case 1: pose_pos.y = value; break;
                    case 2: pose_pos.z = value; break;
                    case 3: pose_rot.x = value; break;
                    case 4: pose_rot.y = value; break;
                    case 5: pose_rot.z = value; break;
                    case 6: pose_rot.w = value; break;
                    case 7: pose_scl.x = value; break;
                    case 8: pose_scl.y = value; break;
                    case 9: pose_scl.z = value; break;
                }
            };

            for (int i = 0; i < 10; ++i) {
                auto value = pose.offsets[i];

                if (pose.channels[i]) {
                    value += data.frames[current_frame_channel] * pose.scales[i];
                    ++current_frame_channel;
                }

                assign_channel_value(i, value);
            }

            auto mat = glm::mat4(1.f);
            mat = glm::translate(mat, pose_pos);
            mat = mat * glm::mat4_cast(glm::normalize(pose_rot));
            mat = glm::scale(mat, pose_scl);

            if (pose.parent >= 0) {
                mat = skele.bone_mats[pose.parent] * mat;
            }

            mat = mat * glm::inverse(skele.bone_mats[joint_index]);

            skele.frame_mats.push_back(mat);
        }
    }

    return skele;
}

auto get_animation_index(const skeleton& skele, const std::string& name) -> std::optional<int> {
    auto iter =
        std::find_if(begin(skele.animations), end(skele.animations), [&](auto& anim) { return anim.name == name; });
    
    if (iter != end(skele.animations)) {
        return iter - begin(skele.animations);
    } else {
        return std::nullopt;
    }
}

auto get_frame(const skeleton& skele, int anim_index, float time) -> std::span<const glm::mat4> {
    auto& anim = skele.animations.at(anim_index);

    auto frame = int(time * anim.framerate);

    if (anim.loop) {
        frame = frame % anim.num_frames;
    } else {
        frame = std::min(frame, anim.num_frames - 1);
    }

    auto bones_per_frame = skele.bone_mats.size();

    auto start = begin(skele.frame_mats) + bones_per_frame * (anim.first_frame + frame);

    return { start, bones_per_frame };
}

auto blend_frames(
    const skeleton& skele, int anim_index, std::span<const glm::mat4> from, std::span<const glm::mat4> to, float time)
    -> std::vector<glm::mat4> {

    auto out = std::vector<glm::mat4>{};
    out.reserve(from.size());

    for (auto i = 0u; i < skele.bone_mats.size(); ++i) {
        auto from_mat = from[i];

        if (skele.bone_parents[i] >= 0) {
            from_mat = out[skele.bone_parents[i]] * from_mat;
        }

        out.push_back(from_mat);
    }

    return out;
}

} // namespace sushi
