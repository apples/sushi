#ifndef SUSHI_MESH_UTILS_HPP
#define SUSHI_MESH_UTILS_HPP

#include "gl.hpp"

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

inline void bind_attrib_float(
    sushi::attrib_location loc, const unique_buffer& buf, GLint size, std::size_t offset, std::array<float, 4> init) {

    glVertexAttrib4fv(static_cast<GLuint>(loc), init.data());

    if (buf) {
        glEnableVertexAttribArray(static_cast<GLuint>(loc));
        glBindBuffer(GL_ARRAY_BUFFER, buf.get());
        glVertexAttribPointer(
            static_cast<GLuint>(loc),
            size,
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<const void*>(offset * size * sizeof(GLfloat)));
    }
}

inline void bind_attrib_ubyte(
    sushi::attrib_location loc,
    const unique_buffer& buf,
    GLint size,
    GLboolean normalize,
    std::size_t offset,
    std::array<std::uint8_t, 4> init) {

    glVertexAttrib4ubv(static_cast<GLuint>(loc), init.data());

    if (buf) {
        glEnableVertexAttribArray(static_cast<GLuint>(loc));
        glBindBuffer(GL_ARRAY_BUFFER, buf.get());
        glVertexAttribPointer(
            static_cast<GLuint>(loc),
            size,
            GL_UNSIGNED_BYTE,
            normalize,
            0,
            reinterpret_cast<const void*>(offset * size * sizeof(GLubyte)));
    }
}

} // _detail

} // sushi

#endif // SUSHI_MESH_UTILS_HPP
