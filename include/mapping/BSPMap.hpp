#pragma once

#include <BSPFile.hpp>
#include <WADFile.hpp>

#include <Mesh.hpp>
#include <Shader.hpp>
#include <glad/glad.h>
#include <Skybox.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

#define ENGINE_WAD_DIR "assets/game/"
#define ENGINE_SKY_DIR "assets/game/sky/"

// goldsrc bsp's have +z as up, converts to [xyz] opengl format
#define BSP_TO_ENGINE_VECTOR(v) glm::vec3(v.x, v.z, -v.y) 

struct BSPSurface {
	Mesh* mesh;
	std::shared_ptr<Texture> texture;
};

struct BSPHullTrace {
	int content;		// hit type
	float fraction;		// fraction of segment completed before collision
	glm::vec3 normal;	// normal of surface collided with
	bool allSolid;		// true if start and end are both in solid, false if either is not in solid
};

class BSPMap {
public:
	BSPMap(const char* bspPath);
	~BSPMap();

	void draw(Shader& shader, Camera& camera);

	// given some line, check where it intersects the clipping hull
	BSPHullTrace traceHullSegment(
		const int nodeIndex, 
		const glm::vec3& segmentStart, 
		const glm::vec3& segmentEnd
	);
	
	BSPFile bsp;
	std::unordered_map<std::string, WADTexture> wadTextureMap;

	std::vector<BSPSurface> opaqueSurfaces;
	std::vector<BSPSurface> alphaTestSurfaces;
	std::vector<BSPSurface> blendedSurfaces;	// NOTE: unimplemented at the moment
	
	Texture* nullTexture = nullptr;

private:

	Skybox skybox;

	// construct texture-binded surfaces for each face in world model
	void buildSurfaces();

	void drawSurface(BSPSurface surface, Shader& shader);

	// creates shared pointer to texture from given bsp entry
	std::shared_ptr<Texture> buildTexture(const BSPTexture& bspTexture);

	// build opengl mesh from bsp face data
	Mesh* buildFaceMesh(const BSPFace& face);

	// creates skybox off of worldspawn data
	void buildSkybox();
};