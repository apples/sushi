#ifndef SUSHI_SKELETON_HPP
#define SUSHI_SKELETON_HPP

#include "common.hpp"
#include "iqm.hpp"
#include "transform.hpp"

#include <string>
#include <optional>
#include <vector>

/// Sushi
namespace sushi {

struct skeleton {
    struct animation {
        std::string name;
        int first_frame;
        int num_frames;
        float framerate;
        bool loop;
    };

    struct bone {
        glm::mat4 base_pose;
        glm::mat4 base_pose_inverse;
        int parent;
        std::string name;
    };

    std::vector<bone> bones;
    std::vector<transform> frame_transforms;
    std::vector<animation> animations;
};

auto load_skeleton(const iqm::iqm_data& data) -> skeleton;

auto get_animation_index(const skeleton& skele, const std::string& name) -> std::optional<int>;

auto get_frame(const skeleton& skele, const skeleton::animation& anim, float time) -> span<const transform>;

auto get_bone_index(const skeleton& skele, const std::string& name) -> std::optional<int>;

} // namespace sushi

#endif // SUSHI_SKELETON_HPP