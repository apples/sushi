//
// Created by Jeramy on 12/31/2015.
//

#include "framebuffer.hpp"

namespace sushi {

framebuffer create_framebuffer(texture_2d color_tex, texture_2d depth_tex) {
    framebuffer rv = {
        std::move(color_tex),
        std::move(depth_tex),
        make_unique_framebuffer()
    };

    glBindFramebuffer(GL_FRAMEBUFFER, rv.handle.get());
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rv.color_tex.handle.get(), 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rv.depth_tex.handle.get(), 0);

    GLenum buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, buffers);

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

    return rv;
}

} // namespace sushi
