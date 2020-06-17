#ifndef SUSHI_TRANSFORM_HPP
#define SUSHI_TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sushi {

struct transform {
    glm::vec3 pos = {0, 0, 0};
    glm::quat rot = {1, 0, 0, 0};
    glm::vec3 scl = {1, 1, 1};
};

auto mix(const transform& a, const transform& b, float t) -> transform;

auto to_mat4(const transform& x) -> glm::mat4;

} // namespace sushi

#endif // SUSHI_TRANSFORM_HPP