#include "pose.hpp"

#include <glm/glm.hpp>

#include <algorithm>

namespace sushi {

pose::pose(const skeleton& skele) : skele(&skele), pose_data(nullpose{}) {}
pose::pose(const skeleton& skele, span<const transform> single) : skele(&skele), pose_data(single) {}
pose::pose(const skeleton& skele, blended_pose_data blended) : skele(&skele), pose_data(blended) {}

void pose::set_uniform(GLint uniform_location) const {
    glm::mat4 mats[32];

    auto sz = std::min(std::size_t{32}, skele->bones.size());

    switch (pose_data.index()) {
        case NULLPOSE: {
            for (auto i = 0; i < sz; ++i) {
                mats[i] = glm::mat4(1.f);
            }
            break;
        }
        case SINGLE: {
            auto& s = std::get<SINGLE>(pose_data);

            for (auto i = 0; i < sz; ++i) {
                auto parent = skele->bones[i].parent;
                auto& mat = mats[i];

                mat = to_mat4(s[i]);

                if (parent >= 0) {
                    mat = mats[parent] * mat;
                }
            }
            break;
        }
        case BLENDED: {
            auto& b = std::get<BLENDED>(pose_data);

            for (auto i = 0; i < sz; ++i) {
                auto parent = skele->bones[i].parent;
                auto& mat = mats[i];

                mat = to_mat4(mix(b.from[i], b.to[i], b.alpha));

                if (parent >= 0) {
                    mat = mats[parent] * mat;
                }
            }
            break;
        }
    }

    for (auto i = 0; i < sz; ++i) {
        mats[i] = mats[i] * skele->bones[i].base_pose_inverse;
    }

    glUniformMatrix4fv(uniform_location, sz, GL_FALSE, glm::value_ptr(mats[0]));
}

auto pose::get_bone_transform(int i) const -> glm::mat4 {
    switch (pose_data.index()) {
        case NULLPOSE: {
            return skele->bones[i].base_pose;
        }
        case SINGLE: {
            auto& s = std::get<SINGLE>(pose_data);

            auto get_mat = [&](const auto& get_mat, int i) -> glm::mat4 {
                auto parent = skele->bones[i].parent;

                auto mat = to_mat4(s[i]);

                if (parent >= 0) {
                    mat = get_mat(get_mat, parent) * mat;
                }

                return mat;
            };

            return get_mat(get_mat, i);
        }
        case BLENDED: {
            auto& b = std::get<BLENDED>(pose_data);

            auto get_mat = [&](const auto& get_mat, int i) -> glm::mat4 {
                auto parent = skele->bones[i].parent;

                auto mat = to_mat4(mix(b.from[i], b.to[i], b.alpha));

                if (parent >= 0) {
                    mat = get_mat(get_mat, parent) * mat;
                }

                return mat;
            };

            return get_mat(get_mat, i);
        }
    }
}

auto get_pose(const skeleton& skele, std::optional<int> anim_index, float time, bool smooth) -> pose {
    if (!anim_index) {
        return pose(skele);
    }

    auto& anim = skele.animations.at(*anim_index);

    auto frame_mats_prev = get_frame(skele, anim, time);

    if (smooth) {
        auto frame_mats_next = get_frame(skele, anim, time + 1.f / anim.framerate);

        auto alpha = time * anim.framerate - std::floor(time * anim.framerate);

        return pose{skele, pose::blended_pose_data{frame_mats_prev, frame_mats_next, alpha}};
    } else {
        return pose{skele, frame_mats_prev};
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
