#include <OpenGL/VAO.h>

VAO::VAO() {
    glad_glGenVertexArrays(1, &ID);
    checkGLError("VAO constructor: Failed to generate VAO");
}

void VAO::LinkAttrib(VBO VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset) {
    VBO.Bind();
    checkGLError("LinkAttrib: Failed to bind VBO");

    glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
    checkGLError("LinkAttrib: Failed to set vertex attribute pointer");

    glEnableVertexAttribArray(layout);
    checkGLError("LinkAttrib: Failed to enable vertex attribute array");

    VBO.Unbind();
    checkGLError("LinkAttrib: Failed to unbind VBO");
}

void VAO::Bind() {
    glBindVertexArray(ID);
    checkGLError("Bind: Failed to bind VAO");
}

void VAO::Unbind() {
    glBindVertexArray(0);
    checkGLError("Unbind: Failed to unbind VAO");
}

void VAO::Delete() {
    Unbind();
    glDeleteVertexArrays(1, &ID);
    checkGLError("Delete: Failed to delete VAO");
}

void VAO::setVertexAttribDivisor(GLuint index, GLuint divisor) {
    glVertexAttribDivisor(index, divisor);
    checkGLError("Failed to set vertex attribute divisor");
}