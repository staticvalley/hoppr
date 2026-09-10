#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/tinyobj/tiny_obj_loader.h"

#include <Model.hpp>
#include <Mesh.hpp>
#include <Shader.hpp>

#include <glm/glm.hpp>
#include <vector>
#include <glm/ext/matrix_transform.hpp>

#include <SDL3/SDL.h>

Model::Model()
	: position(0.0f),
	  rotation(0.0f),
	  scale(1.0f)
{}

void Model::addMesh(Mesh mesh) {
	meshList.push_back(std::move(mesh)); // move ownership of mesh to Model
}

void Model::setTexture(Texture* texture) {
	for (Mesh& mesh : meshList)
		mesh.material.diffuseTexture = texture;
}

void Model::draw(Shader& shader) {
	shader.setUniformMat4("u_model", getModelMatrix());
	for (Mesh& mesh : meshList)
		mesh.draw(shader);
}

glm::mat4 Model::getModelMatrix() {
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
	model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
	model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
	model = glm::scale(model, scale);
	return model;
}

void Model::setRotationDegrees(glm::vec3 rotationTransform) {
	rotation = glm::radians(rotationTransform);
}

void Model::updateBoundingBox() {
	glm::vec3 minVertex(FLT_MAX);
	glm::vec3 maxVertex(-FLT_MAX);

	// find min and max vertices for box corners
	for (Mesh& mesh : meshList) {
		for (Vertex& vertex : mesh.vertices) {
			glm::vec3 vertexWorldPosition = glm::vec3(getModelMatrix() * glm::vec4(vertex.position, 1.0f));
			minVertex = glm::min(minVertex, vertexWorldPosition);
			maxVertex = glm::max(maxVertex, vertexWorldPosition);
		}
	}

	boundingBox = { minVertex, maxVertex };
}

Model* Model::loadOBJ(const char* filePath) {
	
	tinyobj::attrib_t attribute;
	
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	
	// warnings and errors coppied into string
	std::string statusMessage;

	// load .obj file from filePath
	bool success = tinyobj::LoadObj(&attribute, &shapes, &materials, &statusMessage, filePath);

	if (!success) {
		SDL_Log("tinyobj loader error: %s", statusMessage.c_str());
		return nullptr;
	}

	Model* model = new Model();

	for (tinyobj::shape_t& shape : shapes) {
		
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		
		for (tinyobj::index_t& index : shape.mesh.indices) {
		
			Vertex vertex;

			// set positions for vertex
			vertex.position = {
				attribute.vertices[3 * index.vertex_index + 0],
				attribute.vertices[3 * index.vertex_index + 1],
				attribute.vertices[3 * index.vertex_index + 2]
			};

			// set normals for vertex
			if (index.normal_index >= 0) {
				vertex.normal = {
					attribute.normals[3 * index.normal_index + 0],
					attribute.normals[3 * index.normal_index + 1],
					attribute.normals[3 * index.normal_index + 2]
				};
			}

			// set uv coords for vertex
			if (index.texcoord_index >= 0) {
				vertex.uv = {
					attribute.texcoords[2 * index.texcoord_index + 0],
					attribute.texcoords[2 * index.texcoord_index + 1]
				};
			}

			// add vertices and indices to lists
			vertices.push_back(vertex);
			indices.push_back((GLuint)indices.size());
		}
		model->addMesh(Mesh(vertices, indices));
	}
	return model;
}
