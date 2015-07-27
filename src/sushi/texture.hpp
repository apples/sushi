//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_TEXTURE_HPP
#define SUSHI_TEXTURE_HPP

#include "gl.hpp"
#include "common.hpp"

#include <lodepng.h>

#include <iostream>
#include <string>

/// Sushi
namespace sushi {

/// Deleter for OpenGL texture objects.
struct texture_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteTextures(1, &buf);
    }
};

/// A unique handle to an OpenGL texture object.
using unique_texture = unique_gl_resource<texture_deleter>;

/// Creates a unique OpenGL texture object.
/// \return A unique texture object.
inline unique_texture make_unique_texture() {
    GLuint buf;
    glGenTextures(1, &buf);
    return unique_texture(buf);
}

/// A 2D texture.
struct texture_2d {
    unique_texture handle;
    int width = 0;
    int height = 0;
};

/// Loads a 2D texture from a PNG file.
/// \param fname File name.
/// \param smooth Request texture smoothing.
/// \param wrap Request texture wrapping.
/// \return The texture represented by the file, or an empty texture if a failure occurs.
texture_2d load_texture_2d(const std::string& fname, bool smooth, bool wrap) {
    std::vector<unsigned char> image;
    unsigned width, height;
    auto error = lodepng::decode(image, width, height, fname);

    texture_2d rv;

    if (error != 0) {
        std::clog << "sushi::load_texture_2d: Warning: Unable to load texture \"" << fname << "\"." << std::endl;
        return rv;
    }

    rv.handle = make_unique_texture();
    rv.width = width;
    rv.height = height;

    glBindTexture(GL_TEXTURE_2D, rv.handle.get());

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (smooth ? GL_LINEAR : GL_NEAREST));
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (smooth ? GL_LINEAR : GL_NEAREST));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);

    return rv;
}

/// Sets the texture for a slot.
/// \param slot Slot index. Must be within the range `[0,GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)`.
/// \param tex The texture to bind.
inline void set_texture(int slot, const texture_2d& tex) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex.handle.get());
}

}

#endif //SUSHI_TEXTURE_HPP
