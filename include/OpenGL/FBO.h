#pragma once

#include <glad/glad.h>
#include <OpenGL/textureClass.h>
#include <utils.h>

class FBO
{
public:
    GLuint ID;
    bool isBindTexture = false;

    FBO(Texture2D texture);

    void Bind();
    void Unbind();
    void Delete();
};
