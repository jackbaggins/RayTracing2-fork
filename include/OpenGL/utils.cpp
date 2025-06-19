#include <utils.h>

void checkGLError(const std::string& message) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error: " << message << ". Error code: " << error << std::endl;
    }
}