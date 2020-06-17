#ifndef SUSHI_MESH_UTILS_HPP
#define SUSHI_MESH_UTILS_HPP

#include "gl.hpp"
#include "attrib_location.hpp"

#include <vector>

namespace sushi {

namespace _detail {

template <typename T>
auto load_buffer(const std::vector<T>& arr) -> unique_buffer {
    if (!arr.empty()) {
        auto buf = make_unique_buffer();
        glBindBuffer(GL_ARRAY_BUFFER, buf.get());
        glBufferData(GL_ARRAY_BUFFER, arr.size() * sizeof(arr[0]), &arr[0], GL_STATIC_DRAW);
        return buf;
    } else {
        return nullptr;
    }
}

inline void bind_attrib(
    sushi::attrib_location loc,
    const unique_buffer& buf,
    GLint size,
    GLenum type,
    GLboolean normalize,
    std::size_t offset,
    std::array<float, 4> init) {

    glVertexAttrib4fv(static_cast<GLuint>(loc), init.data());

    if (buf) {
        glEnableVertexAttribArray(static_cast<GLuint>(loc));
        glBindBuffer(GL_ARRAY_BUFFER, buf.get());
        glVertexAttribPointer(
            static_cast<GLuint>(loc),
            size,
            type,
            normalize,
            0,
            reinterpret_cast<const void*>(offset * size * sizeof(GLfloat)));
    }
}

} // _detail

} // sushi

#endif // SUSHI_MESH_UTILS_HPP
