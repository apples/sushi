//
// Created by Jeramy on 8/22/2015.
//

#include "texture.hpp"

#include <lodepng.h>

#include <iostream>

namespace {

constexpr GLenum get_source_type(sushi::TexType type) {
    switch (type) {
        case sushi::TexType::COLOR: return GL_RGB;
        case sushi::TexType::COLORA: return GL_RGBA;
        case sushi::TexType::UCOLOR: return GL_RGBA_INTEGER;
        case sushi::TexType::FLOAT3: return GL_RGB;
        case sushi::TexType::DEPTH: return GL_DEPTH_COMPONENT;
    }
}

} // namespace <anonymous>

namespace sushi {

texture_2d load_texture_2d(const std::string& fname, bool smooth, bool wrap, bool anisotropy, TexType type) {
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

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (smooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST));
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (smooth ? GL_LINEAR : GL_NEAREST));
    glTexImage2D(GL_TEXTURE_2D, 0, GLint(type), width, height, 0, get_source_type(type), GL_UNSIGNED_BYTE, &image[0]);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (anisotropy) {
        float max_anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
    }

    return rv;
}

texture_2d create_uninitialized_texture_2d(int width, int height, TexType type) {
    texture_2d rv = {make_unique_texture(), width, height};
    sushi::set_texture(0, rv);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GLint(type), width, height, 0, get_source_type(type), GL_UNSIGNED_BYTE, nullptr);
    return rv;
}

} // namespace sushi
