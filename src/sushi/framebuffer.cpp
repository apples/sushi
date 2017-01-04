//
// Created by Jeramy on 12/31/2015.
//

#include "framebuffer.hpp"

#include <numeric>

namespace sushi {

framebuffer create_framebuffer(std::vector<texture_2d> color_texs, texture_2d depth_tex) {
    framebuffer rv = {
        depth_tex.width,
        depth_tex.height,
        std::move(color_texs),
        std::move(depth_tex),
        make_unique_framebuffer()
    };

    set_framebuffer(rv);

    for (int i=0; i<rv.color_texs.size(); ++i) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, rv.color_texs[i].handle.get(), 0);
    }

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rv.depth_tex.handle.get(), 0);

    std::vector<GLenum> buffers(rv.color_texs.size());
    std::iota(begin(buffers), end(buffers), GL_COLOR_ATTACHMENT0);
    glDrawBuffers(buffers.size(), &buffers[0]);

    switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            throw std::runtime_error("Failed to create framebuffer: Texture size mismatch!");
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            throw std::runtime_error("Failed to create framebuffer: Incomplete attachments!");
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            throw std::runtime_error("Failed to create framebuffer: Missing attachments!");
        case GL_FRAMEBUFFER_UNSUPPORTED:
            throw std::runtime_error("Failed to create framebuffer: Unsupported format!");
        default:
            throw std::runtime_error("Failed to create framebuffer: Unknown error!");
    }

    set_framebuffer(nullptr);

    return rv;
}

} // namespace sushi
