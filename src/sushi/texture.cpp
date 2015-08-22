//
// Created by Jeramy on 8/22/2015.
//

#include "texture.hpp"

#include <lodepng.h>

#include <iostream>

namespace sushi {

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

}
