#ifndef SUSHI_POSE_HPP
#define SUSHI_POSE_HPP

#include "common.hpp"
#include "iqm.hpp"
#include "transform.hpp"
#include "skeleton.hpp"
#include "mesh_group.hpp"

#include <string>
#include <optional>
#include <variant>
#include <vector>

/// Sushi
namespace sushi {

class pose {
public:
    struct blended_pose_data {
        span<const transform> from;
        span<const transform> to;
        float alpha;
    };

    pose() = default;
    pose(const skeleton& skele, span<const transform> single);
    pose(const skeleton& skele, blended_pose_data blended);

    void set_uniform(GLint uniform_location) const;

    auto get_bone_transform(int i) const -> glm::mat4;

private:
    const skeleton* skele = nullptr;
    std::variant<span<const transform>, blended_pose_data> pose_data;
};

auto get_pose(const skeleton* skele, std::optional<int> anim_index, float time, bool smooth) -> std::optional<pose>;

void draw_mesh(const mesh_group& group, const pose& pose);

} // namespace sushi

#endif // SUSHI_POSE_HPP