#pragma once

#include <BSPFile.hpp>
#include <WADFile.hpp>

#include <Mesh.hpp>
#include <Shader.hpp>
#include <Skybox.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <BSPBrushEntity.hpp>
#include <Util.hpp>

#include "Log.hpp"

#define ENGINE_WAD_DIR "assets/game/"
#define ENGINE_SKY_DIR "assets/game/sky/"

struct BSPHullTrace {
	int content;		// hit type
	float fraction;		// fraction of segment completed before collision
	glm::vec3 normal;	// normal of surface collided with
	bool allSolid;		// true if start and end are both in solid, false if either is not in solid
};

// print formatter for BSP traces
template <>
struct std::formatter<BSPHullTrace> : std::formatter<char> {
	auto format(const BSPHullTrace& trace, std::format_context& ctx) const {
		return std::format_to(
			ctx.out(),
			"BSPHullTrace: {{\n"
			"  .content = {} ({}),\n"
			"  .fraction = {},\n"
			"  .normal = {},\n"
			"  .allSolid = {},\n"
			"}}",
			BSPContentsToString(trace.content),
			trace.content,
			trace.fraction,
			trace.normal,
			trace.allSolid
		);
	}
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

	std::vector<Mesh*> opaqueSurfaces;
	std::vector<Mesh*> alphaTestSurfaces;
	std::vector<Mesh*> blendedSurfaces;	// NOTE: unimplemented at the moment

	std::vector<BSPBrushEntity> brushEntities;
	
	Texture* nullTexture = nullptr;

private:

	Skybox skybox;

	// construct texture-binded surfaces for each face in world model
	void buildSurfaces();

	// creates shared pointer to texture from given bsp entry
	std::shared_ptr<Texture> buildTexture(const BSPTexture& bspTexture);

	// build opengl mesh from bsp face data
	Mesh* buildFaceMesh(const BSPFace& face);

	// creates skybox off of worldspawn data
	void buildSkybox();
};