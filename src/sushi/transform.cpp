#include "transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sushi {

auto mix(const transform& a, const transform& b, float t) -> transform {
    return {
        mix(a.pos, b.pos, t),
        slerp(a.rot, b.rot, t),
        mix(a.scl, b.scl, t),
    };
}

auto to_mat4(const transform& x) -> glm::mat4 {
    auto mat = glm::mat4(1.f);
    mat = glm::translate(mat, x.pos);
    mat = mat * glm::mat4_cast(x.rot);
    mat = glm::scale(mat, x.scl);
    return mat;
}

} // namespace sushi
