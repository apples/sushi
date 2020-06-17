#include "pose.hpp"

#include <glm/glm.hpp>

#include <algorithm>

namespace sushi {

pose::pose(const skeleton& skele, span<const transform> single) : skele(&skele), pose_data(single) {}
pose::pose(const skeleton& skele, blended_pose_data blended) : skele(&skele), pose_data(blended) {}

void pose::set_uniform(GLint uniform_location) const {
    std::visit(overload{
        [&](const span<const transform>& s) {
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

                auto blended_transform = mix(b.from[i], b.to[i], b.alpha);

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
    return std::visit(overload{
        [&](const span<const transform>& s) -> glm::mat4 {
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

                auto blended_transform = mix(b.from[i], b.to[i], b.alpha);

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

} // namespace sushi
