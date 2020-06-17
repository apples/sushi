//
// Created by Jeramy on 8/22/2015.
//

#include "mesh.hpp"

#include "mesh_utils.hpp"

#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <fstream>
#include <vector>

namespace sushi {

pose::pose(const skeleton& skele, span<const skeleton::transform> single) : skele(&skele), pose_data(single) {}
pose::pose(const skeleton& skele, blended_pose_data blended) : skele(&skele), pose_data(std::move(blended)) {}

void pose::set_uniform(GLint uniform_location) const {
    std::visit(overload{
        [&](const span<const skeleton::transform>& s) {
            glm::mat4 mats[32];

            auto sz = std::min(std::size_t{32}, s.size());

            for (auto i = 0; i < sz; ++i) {
                auto parent = skele->bones[i].parent;
                auto& mat = mats[i];

                mat = to_mat4(s[i]);

                if (parent >= 0) {
                    mat = mats[parent] * skele->bones[parent].base_pose * mat * skele->bones[i].base_pose_inverse;
                } else {
                    mat = mat * skele->bones[i].base_pose_inverse;
                }
            }

            glUniformMatrix4fv(uniform_location, sz, GL_FALSE, glm::value_ptr(mats[0]));
        },
        [&](const blended_pose_data& b) {
            glm::mat4 mats[32];

            auto sz = std::min({std::size_t{32}, b.from.size(), b.to.size()});

            for (auto i = 0; i < sz; ++i) {
                auto parent = skele->bones[i].parent;
                auto& mat = mats[i];

                auto blended_transform = lerp(b.from[i], b.to[i], b.alpha);

                mat = to_mat4(blended_transform);

                if (parent >= 0) {
                    mat = mats[parent] * skele->bones[parent].base_pose * mat * skele->bones[i].base_pose_inverse;
                } else {
                    mat = mat * skele->bones[i].base_pose_inverse;
                }
            }

            glUniformMatrix4fv(uniform_location, sz, GL_FALSE, glm::value_ptr(mats[0]));
        }
    }, pose_data);
}

auto pose::get_bone_transform(int i) const -> glm::mat4 {
    std::visit(overload{
        [&](const span<const skeleton::transform>& s) -> glm::mat4 {
            auto get_mat = [&](auto& get_mat, int i) -> glm::mat4 {
                auto parent = skele->bones[i].parent;

                auto mat = to_mat4(s[i]);

                if (parent >= 0) {
                    mat = get_mat(get_mat, parent) * skele->bones[parent].base_pose * mat * skele->bones[i].base_pose_inverse;
                } else {
                    mat = mat * skele->bones[i].base_pose_inverse;
                }

                return mat;
            };

            return get_mat(get_mat, i);
        },
        [&](const blended_pose_data& b) -> glm::mat4 {
            auto get_mat = [&](auto& get_mat, int i) -> glm::mat4 {
                auto parent = skele->bones[i].parent;

                auto blended_transform = lerp(b.from[i], b.to[i], b.alpha);

                auto mat = to_mat4(blended_transform);

                if (parent >= 0) {
                    mat = get_mat(get_mat, parent) * skele->bones[parent].base_pose * mat * skele->bones[i].base_pose_inverse;
                } else {
                    mat = mat * skele->bones[i].base_pose_inverse;
                }

                return mat;
            };

            return get_mat(get_mat, i);
        }
    }, pose_data);
}

auto to_mat4(const skeleton::transform& t) -> glm::mat4 {
    auto m = glm::mat4(1.f);
    m = glm::translate(m, t.pos);
    m = m * glm::mat4_cast(t.rot);
    m = glm::scale(m, t.scl);
    return m;
}

auto lerp(const skeleton::transform& a, const skeleton::transform& b, float alpha) -> skeleton::transform {
    return {
        mix(a.pos, b.pos, alpha),
        slerp(a.rot, b.rot, alpha),
        mix(a.scl, b.scl, alpha),
    };
}

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
    constexpr bool orient90X = true;
    const auto rotfixer90X = glm::angleAxis(glm::radians(-90.f), glm::vec3{1, 0, 0});

    skeleton skele;

    // Bones

    skele.bones.reserve(data.joints.size());

    for (const auto& iqm_joint : data.joints) {

        auto mat = glm::mat4(1.f);
        mat = glm::translate(mat, iqm_joint.pos);
        mat = mat * glm::mat4_cast(glm::normalize(iqm_joint.rot));
        mat = glm::scale(mat, iqm_joint.scl);

        if (iqm_joint.parent >= 0) {
            mat = skele.bones[iqm_joint.parent].base_pose * mat;
        }

        skeleton::bone b;
        b.base_pose = mat;
        b.base_pose_inverse = glm::inverse(mat);
        b.parent = iqm_joint.parent;
        b.name = iqm_joint.name;

        skele.bones.push_back(std::move(b));
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

    if (!data.frames.empty() && data.num_framechannels > 0) {
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

                if (orient90X && skele.bones[joint_index].parent == -1) {
                    pose_pos = rotfixer90X * pose_pos;
                    pose_rot = rotfixer90X * pose_rot;
                }

                skele.frame_transforms.push_back({ pose_pos, pose_rot, pose_scl });
            }
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

    auto bones_per_frame = skele.bones.size();

    auto start = begin(skele.frame_transforms) + bones_per_frame * (anim.first_frame + frame);

    return span(&*start, bones_per_frame);
}

auto get_bone_index(const skeleton& skele, const std::string& name) -> std::optional<int> {
    for (auto i = 0; i < skele.bones.size(); ++i) {
        if (skele.bones[i].name == name) {
            return i;
        }
    }

    return std::nullopt;
}

auto get_pose(const skeleton* skele, std::optional<int> anim_index, float time, bool smooth) -> std::optional<pose> {
    if (!skele || !anim_index) {
        return std::nullopt;
    }

    auto& anim = skele->animations.at(*anim_index);

    auto frame_mats_prev = get_frame(*skele, anim, time);

    if (smooth) {
        auto frame_mats_next = get_frame(*skele, anim, time + 1.f / anim.framerate);

        auto alpha = time * anim.framerate - std::floor(time * anim.framerate);

        return pose{*skele, pose::blended_pose_data{frame_mats_prev, frame_mats_next, alpha}};
    } else {
        return pose{*skele, frame_mats_prev};
    }
}

void draw_mesh(const mesh_group& group, const pose& pose) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto animated_uniform = glGetUniformLocation(program, "Animated");

    auto bones_uniform = glGetUniformLocation(program, "Bones");

    glUniform1i(animated_uniform, 1);
    pose.set_uniform(bones_uniform);

    SUSHI_DEFER { glBindVertexArray(0); };

    for (const auto& mesh : group.meshes) {
        glBindVertexArray(mesh.vao.get());
        glDrawElements(GL_TRIANGLES, mesh.num_tris * 3, GL_UNSIGNED_INT, nullptr);
    }
}

void draw_mesh(const mesh_group& group, const skeleton* skele, std::optional<int> anim_index, float time, bool smooth) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto animated_uniform = glGetUniformLocation(program, "Animated");

    auto pose = get_pose(skele, anim_index, time, smooth);

    if (pose) {
        auto bones_uniform = glGetUniformLocation(program, "Bones");

        glUniform1i(animated_uniform, 1);
        pose->set_uniform(bones_uniform);
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
