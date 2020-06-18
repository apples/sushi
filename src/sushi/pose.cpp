#include "pose.hpp"

#include <glm/glm.hpp>

#include <algorithm>

namespace sushi {

namespace {

template <typename T, typename U>
auto compute_pose_matrix(const sushi::skeleton& skele, int i, T&& get_pose_mat, U&& get_parent_mat) {
    auto parent = skele.bones[i].parent;

    auto mat = to_mat4(get_pose_mat(i));

    if (parent >= 0) {
        mat = get_parent_mat(parent) * skele.bones[parent].base_pose * mat * skele.bones[i].base_pose_inverse;
    } else {
        mat = mat * skele.bones[i].base_pose_inverse;
    }

    return mat;
}

} // namespace

pose::pose(const skeleton& skele, span<const transform> single) : skele(&skele), pose_data(single) {}
pose::pose(const skeleton& skele, blended_pose_data blended) : skele(&skele), pose_data(blended) {}

void pose::set_uniform(GLint uniform_location) const {
    auto impl = [&](auto size, const auto& get_pose_mat) {
        glm::mat4 mats[32];
        
        auto get_parent_mat = [&](int p) { return mats[p]; };

        auto sz = std::min(std::size_t{32}, size);

        for (auto i = 0; i < sz; ++i) {
            mats[i] = compute_pose_matrix(*skele, i, get_pose_mat, get_parent_mat);
        }

        glUniformMatrix4fv(uniform_location, sz, GL_FALSE, glm::value_ptr(mats[0]));
    };

    std::visit(
        overload{
            [&](const span<const transform>& s) { impl(s.size(), [&](int i) { return s[i]; }); },
            [&](const blended_pose_data& b) {
                auto sz = std::min(b.from.size(), b.to.size());
                impl(sz, [&](int i) { return mix(b.from[i], b.to[i], b.alpha); });
            },
        },
        pose_data);
}

auto pose::get_bone_transform(int i) const -> glm::mat4 {
    auto impl = [&](const auto& get_pose_mat) {
        auto get_mat = [&](const auto& get_mat, int i) -> glm::mat4 {
            auto get_parent_mat = [&](int p) { return get_mat(get_mat, p); };

            return compute_pose_matrix(*skele, i, get_pose_mat, get_parent_mat);
        };

        return get_mat(get_mat, i);
    };

    auto from_bind_pose = std::visit(
        overload{
            [&](const span<const transform>& s) { return impl([&](int i) { return s[i]; }); },
            [&](const blended_pose_data& b) { return impl([&](int i) { return mix(b.from[i], b.to[i], b.alpha); }); },
        },
        pose_data);
    
    return from_bind_pose * skele->bones[i].base_pose;
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
