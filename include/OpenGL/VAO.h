#pragma once

#include <iostream>

#include<glad/glad.h>
#include <VBO.h>
#include <utils.h>

class VAO
{
public:
	GLuint ID;

	VAO();

	void LinkAttrib(VBO VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset);

	void Bind();

	void Unbind();

	void Delete();

	void setVertexAttribDivisor(GLuint index, GLuint divisor);
};