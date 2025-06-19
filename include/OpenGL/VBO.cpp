#include <OpenGL/VBO.h>

VBO::VBO(GLfloat* vertices, GLsizeiptr size) {
    glGenBuffers(1, &ID);
    checkGLError("Failed to generate buffer");

    glBindBuffer(GL_ARRAY_BUFFER, ID);
    checkGLError("Failed to bind buffer");

    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    checkGLError("Failed to set buffer data");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGLError("Failed to unbind buffer");
}

VBO::VBO(std::vector<Vertex3D>& vertices) {
    glGenBuffers(1, &ID);
    checkGLError("Failed to generate buffer");

    glBindBuffer(GL_ARRAY_BUFFER, ID);
    checkGLError("Failed to bind buffer");

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D), vertices.data(), GL_STATIC_DRAW);
    checkGLError("Failed to set buffer data");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGLError("Failed to unbind buffer");
}

VBO::VBO(GLsizeiptr size, GLenum usage) {
    glGenBuffers(1, &ID);
    checkGLError("Failed to generate buffer");

    glBindBuffer(GL_ARRAY_BUFFER, ID);
    checkGLError("Failed to bind buffer");

    glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
    checkGLError("Failed to allocate buffer data");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGLError("Failed to unbind buffer");
}

void VBO::updateData(GLsizeiptr size, const void* data) {
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    checkGLError("Failed to bind VBO");

    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    checkGLError("Failed to update VBO data");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGLError("Failed to unbind VBO");
}

void VBO::Bind() {
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    checkGLError("Failed to bind buffer");
}

void VBO::Unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGLError("Failed to unbind buffer");
}

void VBO::Delete() {
    Unbind();
    glDeleteBuffers(1, &ID);
    checkGLError("Failed to delete buffer");
}
