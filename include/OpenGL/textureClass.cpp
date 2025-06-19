#include <OpenGL/textureClass.h>

Texture2D::Texture2D(int width, int height, const void* pixels, int mipmapLevel, GLenum pixelFormat, GLint filterMode, GLint wrapMode, GLenum textureUnit)
{
    unit = textureUnit;
    glActiveTexture(unit);
    glGenTextures(1, &ID);
    checkGLError("Failed to generate texture");

    glBindTexture(GL_TEXTURE_2D, ID);
    checkGLError("Failed to bind texture");

    glTexImage2D(GL_TEXTURE_2D, mipmapLevel, pixelFormat, width, height, 0, pixelFormat, GL_UNSIGNED_BYTE, pixels);
    checkGLError("Failed to set texture image");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
    checkGLError("Failed to set min filter");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
    checkGLError("Failed to set mag filter");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    checkGLError("Failed to set wrap S");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    checkGLError("Failed to set wrap T");

    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLError("Failed to unbind texture");
}

Texture2D::Texture2D(int width, int height, GLenum textureUnit)
{
    unit = textureUnit;
    glActiveTexture(unit);
    glGenTextures(1, &ID);
    checkGLError("Failed to generate texture");

    glBindTexture(GL_TEXTURE_2D, ID);
    checkGLError("Failed to bind texture");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    checkGLError("Failed to set texture image");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGLError("Failed to set min filter");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGLError("Failed to set mag filter");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    checkGLError("Failed to set wrap S");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGLError("Failed to set wrap T");

    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLError("Failed to unbind texture");
}

Texture2D::Texture2D(const std::string& path, GLenum textureUnit)
{
    unit = textureUnit;
    glActiveTexture(unit);
    glGenTextures(1, &ID);
    checkGLError("Failed to generate texture");

    glBindTexture(GL_TEXTURE_2D, ID);
    checkGLError("Failed to bind texture");

    stbi_set_flip_vertically_on_load(true);

    int width, height, numColCh;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &numColCh, 0);
    if (pixels)
    {
        std::cout << "Texture's number of color channels: " << numColCh << std::endl;

        GLenum format;
        if (numColCh == 1) format = GL_RED;
        else if (numColCh == 2) format = GL_RG;
        else if (numColCh == 3) format = GL_RGB;
        else if (numColCh == 4) format = GL_RGBA;
        else {
            std::cerr << "Unsupported number of color channels." << std::endl;
            stbi_image_free(pixels);
            throw std::runtime_error("Unsupported color channels");
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
        checkGLError("Failed to set texture image");

        if (numColCh == 1)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        checkGLError("Failed to set min filter");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        checkGLError("Failed to set mag filter");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        checkGLError("Failed to set wrap S");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        checkGLError("Failed to set wrap T");

        stbi_image_free(pixels);
    }
    else
    {
        std::cerr << "Failed to load texture" << std::endl;
        throw std::runtime_error("Failed to load texture");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLError("Failed to unbind texture");
}

glm::vec3 Texture2D::readPixel(const glm::vec2& uv)
{
    // Note: This function reads from the framebuffer, not directly from the texture.
    // To read from the texture, consider using a shader or a framebuffer object.
    GLubyte pixel[3];
    glBindTexture(GL_TEXTURE_2D, ID);
    glReadPixels(uv.x, uv.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    return glm::vec3(pixel[0], pixel[1], pixel[2]);
}

void Texture2D::Bind()
{
    glBindTexture(GL_TEXTURE_2D, ID);
    checkGLError("Failed to bind texture");
}

void Texture2D::SetActive()
{
    glActiveTexture(unit);
    checkGLError("Failed to activate texture unit");
}

void Texture2D::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLError("Failed to unbind texture");
}

void Texture2D::Delete()
{
    glDeleteTextures(1, &ID);
    checkGLError("Failed to delete texture");
}
