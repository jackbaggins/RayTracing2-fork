#include <OpenGL/FBO.h>

FBO::FBO(Texture2D texture)
{
    glGenFramebuffers(1, &ID);
    checkGLError("Failed to generate framebuffer");

    glBindFramebuffer(GL_FRAMEBUFFER, ID);
    checkGLError("Failed to bind framebuffer");

    texture.Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.ID, 0);
    checkGLError("Failed to attach texture to framebuffer");

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
        throw std::runtime_error("Framebuffer not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError("Failed to unbind framebuffer");

    isBindTexture = true;
}

void FBO::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ID);
    checkGLError("Failed to bind framebuffer");
}

void FBO::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError("Failed to unbind framebuffer");
}

void FBO::Delete()
{
    glDeleteFramebuffers(1, &ID);
    checkGLError("Failed to delete framebuffer");
}
