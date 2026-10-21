#include <Vertex.hpp>
#include <Mesh.hpp>
#include <Material.hpp>

#include <glad/glad.h>
#include <vector>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices) 
	: vao(0),
	  vbo(0),
	  ebo(0),
	  count((GLsizei)indices.size()),
	  vertices(vertices)
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	// upload vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	// upload index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	// set attribute arrays in vertex array

	// position (layout 1)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

	// normals (layout 2)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	// uv (layout 3)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

	// unbind vao from context
	glBindVertexArray(0);
}

Mesh::Mesh(Mesh&& other) noexcept
	:	material(other.material),
		vertices(std::move(other.vertices)),
		vao(other.vao),
		vbo(other.vbo),
		ebo(other.ebo),
		count(other.count)
{
	// clear values on rhs object to transfer single ownership
	other.vao = 0;
	other.vbo = 0;
	other.ebo = 0;
	other.count = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
	if (this != &other) {

		// delete any arrays or buffers from lhs object
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);

		// assign values from rhs object
		material = other.material;
		vertices = std::move(other.vertices);
		vao = other.vao;
		vbo = other.vbo;
		ebo = other.ebo;
		count = other.count;

		// clear values on rhs object to transfer single ownership
		other.vao = 0;
		other.vbo = 0;
		other.ebo = 0;
		other.count = 0;
		other.vertices.clear();

	}
	return *this;
}

Mesh::~Mesh() {
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
}

void Mesh::draw(Shader& shader) {
	material.apply(shader);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}