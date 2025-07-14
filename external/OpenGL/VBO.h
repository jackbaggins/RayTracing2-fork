#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <utils.h>

#include <iostream>
#include <vector>

struct Vertex3D
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec3 texUV;
};

class VBO
{
public:
	GLuint ID;

	VBO(std::vector<Vertex3D>& vertices);

	VBO(GLfloat* vertices, GLsizeiptr size);

	VBO(GLsizeiptr size, GLenum usage);

	void updateData(GLsizeiptr size, const void* data);

	void Bind();

	void Unbind();

	void Delete();
};