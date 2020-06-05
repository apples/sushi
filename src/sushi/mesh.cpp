//
// Created by Jeramy on 8/22/2015.
//

#include "mesh.hpp"

#include "mesh_utils.hpp"

#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>

namespace sushi {

auto load_meshes(const iqm::iqm_data& data) -> mesh_group {
    using _detail::load_buffer;
    using _detail::bind_attrib;

    mesh_group group;

    group.position_buffer = load_buffer(data.vertexarrays.position);
    group.texcoord_buffer = load_buffer(data.vertexarrays.texcoord);
    group.normal_buffer = load_buffer(data.vertexarrays.normal);
    group.tangent_buffer = load_buffer(data.vertexarrays.tangent);
    group.blendindices_buffer = load_buffer(data.vertexarrays.blendindexes);
    group.blendweights_buffer = load_buffer(data.vertexarrays.blendweights);
    group.color_buffer = load_buffer(data.vertexarrays.color);

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

        bind_attrib(attrib_location::POSITION, group.position_buffer, 3, GL_FLOAT, false, iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::TEXCOORD, group.texcoord_buffer, 2, GL_FLOAT, false,  iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::NORMAL, group.normal_buffer, 3, GL_FLOAT, false,  iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::TANGENT, group.tangent_buffer, 3, GL_FLOAT, false,  iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::BLENDINDICES, group.blendindices_buffer, 4, GL_UNSIGNED_BYTE, GL_FALSE, iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::BLENDWEIGHTS, group.blendweights_buffer, 4, GL_UNSIGNED_BYTE, GL_TRUE, iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::COLOR, group.color_buffer, 4, GL_UNSIGNED_BYTE, GL_TRUE, iqm_mesh.first_vertex, {255, 255, 255, 255});

        group.meshes.push_back(std::move(mesh));
    }

    return group;
}

auto load_skeleton(const iqm::iqm_data& data) -> skeleton {
    skeleton skele;

    // Bones

    skele.bone_parents.reserve(data.joints.size());
    skele.bone_mats.reserve(data.joints.size());
    skele.bone_mats_inverse.reserve(data.joints.size());

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
        skele.bone_mats_inverse.push_back(glm::inverse(mat));
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

            auto pose_pos = glm::vec3{0, 0, 0};
            auto pose_rot = glm::quat{1, 0, 0, 0};
            auto pose_scl = glm::vec3{1, 1, 1};

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

            skele.frame_transforms.push_back({ pose_pos, pose_rot, pose_scl });
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

auto get_frame(const skeleton& skele, const skeleton::animation& anim, float time) -> span<const skeleton::transform> {
    auto frame = int(time * anim.framerate);

    if (anim.loop) {
        frame = frame % anim.num_frames;
    } else {
        frame = std::min(frame, anim.num_frames - 1);
    }

    auto bones_per_frame = skele.bone_mats.size();

    auto start = begin(skele.frame_transforms) + bones_per_frame * (anim.first_frame + frame);

    return span(&*start, bones_per_frame);
}

auto blend_frames(
    const skeleton& skele, span<const skeleton::transform> from, span<const skeleton::transform> to, float alpha)
    -> std::vector<glm::mat4> {

    auto out = std::vector<glm::mat4>{};
    out.reserve(from.size());

    for (auto i = 0u; i < skele.bone_parents.size(); ++i) {
        auto& parent = skele.bone_parents[i];
        auto& from_transform = from[i];
        auto& to_transform = to[i];

        auto mat = glm::mat4(1.f);

        if (alpha != 0.f) {
            mat = glm::translate(mat, glm::mix(from_transform.pos, to_transform.pos, alpha));
            mat = mat * glm::mat4_cast(glm::slerp(from_transform.rot, to_transform.rot, alpha));
            mat = glm::scale(mat, glm::mix(from_transform.scl, to_transform.scl, alpha));
        } else {
            mat = glm::translate(mat, from_transform.pos);
            mat = mat * glm::mat4_cast(from_transform.rot);
            mat = glm::scale(mat, from_transform.scl);
        }

        if (parent >= 0) {
            mat = out[parent] * skele.bone_mats[parent] * mat * skele.bone_mats_inverse[i];
        } else {
            mat = mat * skele.bone_mats_inverse[i];
        }

        out.push_back(mat);
    }

    return out;
}

void draw_mesh(const mesh_group& group, const skeleton* skele, std::optional<int> anim_index, float time, bool smooth) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto animated_uniform = glGetUniformLocation(program, "Animated");

    if (skele && anim_index) {
        auto& anim = skele->animations.at(*anim_index);

        auto frame_mats_prev = get_frame(*skele, anim, time);
        auto frame_mats_next = get_frame(*skele, anim, time + 1.f / anim.framerate);

        auto alpha = smooth ? time * anim.framerate - std::floor(time * anim.framerate) : 0.f;
        auto blended_frame_mats = blend_frames(*skele, frame_mats_prev, frame_mats_next, alpha);

        auto bones_uniform = glGetUniformLocation(program, "Bones");

        glUniform1i(animated_uniform, 1);
        glUniformMatrix4fv(bones_uniform, blended_frame_mats.size(), GL_FALSE, glm::value_ptr(blended_frame_mats[0]));
    } else {
        glUniform1i(animated_uniform, 0);
    }

    SUSHI_DEFER { glBindVertexArray(0); };

    for (const auto& mesh : group.meshes) {
        glBindVertexArray(mesh.vao.get());
        glDrawElements(GL_TRIANGLES, mesh.num_tris * 3, GL_UNSIGNED_INT, nullptr);
    }
}

void draw_mesh(const mesh_group& group) {
    draw_mesh(group, nullptr, std::nullopt, 0.f, false);
}

} // namespace sushi
