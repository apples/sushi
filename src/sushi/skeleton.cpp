#include "skeleton.hpp"

#include <algorithm>

namespace sushi {

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

auto get_frame(const skeleton& skele, const skeleton::animation& anim, float time) -> span<const transform> {
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

} // namespace sushi
