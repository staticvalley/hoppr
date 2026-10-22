#include <BSPMap.hpp>
#include <BSPFile.hpp>
#include <WADFile.hpp>
#include <string>
#include <memory>
#include <Skybox.hpp>
#include <SDL3/SDL.h>
#include <Log.hpp>
#include <BSPBrushEntity.hpp>
#include <Util.hpp>

BSPMap::BSPMap(const char* bspPath)
{
	if (!bsp.load(bspPath))
		return;

	// get WAD paths
	std::vector<std::string> wadPaths = bsp.requiredWADs();

	// read all WAD paths needed by bsp file
	for (std::string wadName : wadPaths) {
		WADFile wad;
		std::string wadPath = ENGINE_WAD_DIR + wadName;
		if (!wad.load(wadPath.c_str())) {
			Log::warn(Log::Channel::MAPPING, "BSPMap: missing \"{}\"", wadPath);
			continue;
		}
		wadTextureMap.merge(wad.takeTextures());
	}

	// compile brush entities
	for (const BSPEntity& entity : bsp.entities()) {
		brushEntities.push_back(BSPBrushEntity(entity));
	}

	buildSkybox();
	buildSurfaces();
}

BSPMap::~BSPMap() {

	for (Mesh* surface : opaqueSurfaces)
		delete surface;

	for (Mesh* surface : alphaTestSurfaces)
		delete surface;

	for (Mesh* surface : blendedSurfaces)
		delete surface;

	delete nullTexture;
}

void BSPMap::draw(Shader& shader, Camera& camera) {
	
	shader.setUniformMat4("u_model", glm::mat4(1.0f));

	for (Mesh* surface : opaqueSurfaces)
		surface->draw(shader);

	for (Mesh* surface : alphaTestSurfaces)
		surface->draw(shader);

	skybox.draw(camera);

	// rebind shader since skybox changes it
	shader.bind();
}

void BSPMap::buildSurfaces() {

	// make cache for texture pointers
	std::unordered_map<int, std::shared_ptr<Texture>> textureCache;

	for (size_t j = 0; j < bsp.models().size(); j++) {

		// model 0 is world geometry model
		// models after 0 are brush entities
		const BSPModel& model = bsp.models()[j];

		// create surface for each face in model
		for (int i = 0; i < model.nFaces; i++) {

			// get given face and texture info for face
			const BSPFace& face = bsp.faces()[model.iFirstFace + i];
			const BSPTextureInfo& texInfo = bsp.textureInfo()[face.iTextureInfo];

			// skip sky and special faces
			if (texInfo.nFlags & 0x1) continue;  // SKY flag

			// get texture referenced by texture info
			const uint32_t textureIndex = texInfo.iMiptex;
			const BSPTexture currentTexture = bsp.textures()[textureIndex];

			// add texture to cache
			if (!textureCache.contains(textureIndex)) {
				// if texture referenced is in an external wad
				if (currentTexture.external) {
					// search for texture name in wad map
					auto search = wadTextureMap.find(currentTexture.name);
					if (search != wadTextureMap.end()) {
						// found, build texture
						WADTexture referencedTexture = wadTextureMap[currentTexture.name];
						textureCache[textureIndex] = buildTexture(referencedTexture);
					} else {
						// texture not found
						textureCache[textureIndex] = nullptr;
						Log::warn(Log::Channel::MAPPING, "Texture not found: {}", currentTexture.name);
					}
				} else {
					// texture in bsp file 
					textureCache[textureIndex] = buildTexture(currentTexture);
				}
			}

			Mesh* surface = buildFaceMesh(face);;
			surface->material.diffuseTexture = textureCache[textureIndex];

			if (textureCache[textureIndex] != nullptr)
				switch (currentTexture.name[0]) {
				case '{':
					alphaTestSurfaces.push_back(surface);
					break;
				default:
					opaqueSurfaces.push_back(surface);
					break;
				}
		}
	}
}

std::shared_ptr<Texture> BSPMap::buildTexture(const BSPTexture& bspTexture) {
		
	if (bspTexture.pixels.empty()) return nullptr;

	// '{' signals texture with transparency in goldsrc
	bool isTransparent = bspTexture.name[0] == '{';

	return std::make_shared<Texture>(
		bspTexture.pixels.data(),
		bspTexture.palette.data(),
		bspTexture.width,
		bspTexture.height,
		isTransparent
	);
}

Mesh* BSPMap::buildFaceMesh(const BSPFace& face) {
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	const BSPTextureInfo& texInfo = bsp.textureInfo()[face.iTextureInfo];
	const BSPTexture& texture = bsp.textures()[texInfo.iMiptex];

	// for lightmap calculations
	std::vector<glm::vec2> baseTexCoords;
	//float minU = std::numeric_limits<float>::max();
	//float maxU = std::numeric_limits<float>::lowest();
	//float minV = std::numeric_limits<float>::max();
	//float maxV = std::numeric_limits<float>::lowest();

	// collect vertices in order
	std::vector<glm::vec3> faceVertices;
	for (int i = 0; i < face.nEdges; i++) {
		
		BSPSurfEdge surfEdge = bsp.surfEdges()[face.iFirstEdge + i];
		BSPEdge edge;
		BSPVertex bspVertex;

		/*
			idk how tf these work or why they are needed, but following the valvesoftware wiki:

			... If the value of the surfedge is positive, the first vertex of the edge is used as vertex for rendering 
			the face, otherwise, the value is multiplied by -1 and the second vertex of the indexed edge is used ...
		*/
		if (surfEdge >= 0)
			edge = bsp.edges()[surfEdge];
		else
			edge = bsp.edges()[-surfEdge];

		if (surfEdge >= 0)
			bspVertex = bsp.vertices()[edge.iVertex[0]];
		else
			bspVertex = bsp.vertices()[edge.iVertex[1]];

		faceVertices.push_back(Conversions::goldsrcToEngineVec(bspVertex));
	}

	const BSPPlane& plane = bsp.planes()[face.iPlane];

	// get normal of face plane in opengl directions
	glm::vec3 normal = Conversions::goldsrcToEngineVec(plane.vNormal);
	
	// flip orientation
	if (face.nPlaneSide) normal = -normal;

	// set all data for single vertex
	for (glm::vec3& position : faceVertices) {
		Vertex vertex;
		vertex.position = position;
		vertex.normal = normal;

		// texture access vectors
		glm::vec3 s = Conversions::goldsrcToEngineVec(texInfo.vS);
		glm::vec3 t = Conversions::goldsrcToEngineVec(texInfo.vT);

		// uv coordinates for vertex
		// 1. project vertex position to texel (s,t) coordinates
		// 2. apply texture offset
		// 3. normalize to 0-1 range for opengl sampling
		float u = (glm::dot(position, s) + texInfo.fSShift) / texture.width;
		float v = (glm::dot(position, t) + texInfo.fTShift) / texture.height;

		vertex.uv = glm::vec2(u, v);
		vertices.push_back(vertex);
	}
	

	// fan triangulate the convex polygon
	// vertex 0 is the hub, connect to each subsequent edge
	for (size_t i = 1; i < faceVertices.size() - 1; i++) {
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i + 1);
	}

	return new Mesh(vertices, indices);
}

void BSPMap::buildSkybox() {

	std::string skyBoxName;

	// get skybox key
	for (const BSPEntity& entity : bsp.entities()) {
		if (entity.contains("classname") && entity.at("classname") == "worldspawn")
			if (entity.contains("skyname"))
				skyBoxName = entity.at("skyname");
	}

	const std::string basePath = std::string(ENGINE_SKY_DIR) + skyBoxName;

	const std::string paths[6] = {
		basePath + "rt.tga",
		basePath + "lf.tga",
		basePath + "up.tga",
		basePath + "dn.tga",
		basePath + "ft.tga",
		basePath + "bk.tga",
	};

	skybox = Skybox(paths, new Shader("BSPMapSkybox", "assets/shaders/skybox.vert", "assets/shaders/skybox.frag"), SKYBOX_TYPE_GOLDSRC);

	Log::info(Log::Channel::MAPPING, "compiled skybox (name: \"{}\")", skyBoxName);
}

BSPHullTrace BSPMap::traceHullSegment(const int nodeIndex, const glm::vec3& segmentStart, const glm::vec3& segmentEnd) {
	
	// epsilon (helps with ramps/inclines)
	const float acceptanceEpsilon = -0.03125f;

	// negative index is a leaf, base case
	if (nodeIndex < 0) {
		bool solid = nodeIndex == BSP_CONTENTS_SOLID;
		return { nodeIndex, solid ? 0.0f : 1.0f, glm::vec3(0.0f), solid };
	}

	const BSPClipNode& cNode = bsp.clipNodes()[nodeIndex];
	const BSPPlane& cPlane = bsp.planes()[cNode.iPlane];
	glm::vec3 planeNormal = Conversions::goldsrcToEngineVec(cPlane.vNormal);

	float segmentStartDistance = glm::dot(
		segmentStart,
		planeNormal
	) - cPlane.fDist;

	float segmentEndDistance = glm::dot(
		segmentEnd,
		planeNormal
	) - cPlane.fDist;

	// if both points are on positive side of plane, recurse front (0)
	if (segmentStartDistance >= acceptanceEpsilon && segmentEndDistance >= acceptanceEpsilon)
		return traceHullSegment(cNode.iChildren[0], segmentStart, segmentEnd);
	
	// if both points are on negative side of plane, recurse back (1)
	if (segmentStartDistance < acceptanceEpsilon && segmentEndDistance < acceptanceEpsilon)
		return traceHullSegment(cNode.iChildren[1], segmentStart, segmentEnd);

	// i think i have computed the line plane intersection correctly
	// https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection

	// lerp for midpoint
	float t = segmentStartDistance / (segmentStartDistance - segmentEndDistance);
	t = glm::clamp(t, 0.0f, 1.0f);
	
	// i_{a} + i_{ab}*t 
	glm::vec3 intersection = segmentStart + t * (segmentEnd - segmentStart);

	int nearSide = segmentStartDistance >= acceptanceEpsilon ? 0 : 1;
	int farSide = segmentStartDistance >= acceptanceEpsilon ? 1 : 0;

	// check nearside
	{
		BSPHullTrace nearTrace = traceHullSegment(cNode.iChildren[nearSide], segmentStart, intersection);
		if (nearTrace.content != BSP_CONTENTS_EMPTY)
			return { nearTrace.content, nearTrace.fraction * t, nearTrace.normal, nearTrace.allSolid };
	}	

	// check farside
	{
		BSPHullTrace farTrace = traceHullSegment(cNode.iChildren[farSide], intersection, segmentEnd);
		if (farTrace.content != BSP_CONTENTS_EMPTY) {
			farTrace.normal = nearSide == 0 ? planeNormal : -planeNormal;
			return { farTrace.content, t + (1.0f - t) * farTrace.fraction, farTrace.normal, false };
		}
	}
		
	return { BSP_CONTENTS_EMPTY, 1.0f, glm::vec3(0.0f), false};
}