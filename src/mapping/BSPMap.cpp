#include <BSPMap.hpp>
#include <BSPFile.hpp>
#include <WADFile.hpp>
#include <string>
#include <print>
#include <memory>
#include <Skybox.hpp>
#include <SDL3/SDL.h>

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
			SDL_Log(std::format("BSPMap: missing \"{}\"", wadPath).c_str());
			continue;
		}
		wadTextureMap.merge(wad.takeTextures());
	}

	buildSkybox();
	buildSurfaces();
}

BSPMap::~BSPMap() {
	for (BSPSurface& surface : opaqueSurfaces)
		delete surface.mesh;

	for (BSPSurface& surface : alphaTestSurfaces)
		delete surface.mesh;

	for (BSPSurface& surface : blendedSurfaces)
		delete surface.mesh;

	delete nullTexture;
}

void BSPMap::draw(Shader& shader, Camera& camera) {
	
	shader.setUniformMat4("u_model", glm::mat4(1.0f));

	// opaque draw pass
	for(BSPSurface& surface : opaqueSurfaces)
		drawSurface(surface, shader);

	// transparent draw pass
	for(BSPSurface surface : alphaTestSurfaces)
		drawSurface(surface, shader);

	skybox.draw(camera);
}

void BSPMap::drawSurface(BSPSurface surface, Shader& shader) {
	
	// if the surface doesnt have a set texture, use nullTexture
	if (surface.texture)
		surface.texture->bind(0);
	else if (nullTexture)
		nullTexture->bind(0);
	else
		return;

	shader.setUniformInt1("u_texture", 0);
	surface.mesh->draw(shader);
}

void BSPMap::buildSurfaces() {

	// make cache for texture pointers
	std::unordered_map<int, std::shared_ptr<Texture>> textureCache;

	for (int j = 0; j < bsp.models().size(); j++) {

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
						std::println("tex not found: {}", currentTexture.name);
					}
				} else {
					// texture in bsp file 
					textureCache[textureIndex] = buildTexture(currentTexture);
				}
			}

			BSPSurface surface;
			surface.mesh = buildFaceMesh(face);
			surface.texture = textureCache[textureIndex];

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

	std::vector<uint8_t> pixel(bspTexture.width * bspTexture.height * (isTransparent ? 4 : 3));

	if (isTransparent){
		// convert paletted pixels to rgba
		for (int p = 0; p < bspTexture.width * bspTexture.height; p++) {
			uint8_t paletteIndex = bspTexture.pixels[p];
			pixel[p * 4 + 0] = bspTexture.palette[paletteIndex * 3 + 0];
			pixel[p * 4 + 1] = bspTexture.palette[paletteIndex * 3 + 1];
			pixel[p * 4 + 2] = bspTexture.palette[paletteIndex * 3 + 2];
			pixel[p * 4 + 3] = (isTransparent && paletteIndex == 255) ? 0 : 255;
		}
	} else {
		for (int p = 0; p < bspTexture.width * bspTexture.height; p++) {
			uint8_t paletteIndex = bspTexture.pixels[p];
			pixel[p * 3 + 0] = bspTexture.palette[paletteIndex * 3 + 0];
			pixel[p * 3 + 1] = bspTexture.palette[paletteIndex * 3 + 1];
			pixel[p * 3 + 2] = bspTexture.palette[paletteIndex * 3 + 2];
		}
	}

	return std::shared_ptr<Texture>(new Texture(pixel.data(), bspTexture.width, bspTexture.height, (isTransparent ? GL_RGBA : GL_RGB)));
}

Mesh* BSPMap::buildFaceMesh(const BSPFace& face) {
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	const BSPTextureInfo& texInfo = bsp.textureInfo()[face.iTextureInfo];
	const BSPTexture& texture = bsp.textures()[texInfo.iMiptex];

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

		faceVertices.push_back(BSP_TO_ENGINE_VECTOR(bspVertex));
	}

	const BSPPlane& plane = bsp.planes()[face.iPlane];

	// get normal of face plane in opengl directions
	glm::vec3 normal = BSP_TO_ENGINE_VECTOR(plane.vNormal);
	
	// flip orientation
	if (face.nPlaneSide) normal = -normal;

	// set all data for single vertex
	for (glm::vec3& position : faceVertices) {
		Vertex vertex;
		vertex.position = position;
		vertex.normal = normal;

		// texture access vectors
		glm::vec3 s = BSP_TO_ENGINE_VECTOR(texInfo.vS);
		glm::vec3 t = BSP_TO_ENGINE_VECTOR(texInfo.vT);

		// uv coordinates for vertex
		// 1. project vertex position to texel (s,t) coordinates
		// 2. bsp scale is inches, engine is in meters
		// 3. apply texture offset
		// 4. normalize to 0-1 range for opengl sampling
		float u = (glm::dot(position, s) + texInfo.fSShift) / texture.width;
		float v = (glm::dot(position, t) + texInfo.fTShift) / texture.height;

		vertex.uv = glm::vec2(u, v);
		vertices.push_back(vertex);
	}
	

	// fan triangulate the convex polygon
	// vertex 0 is the hub, connect to each subsequent edge
	for (int i = 1; i < faceVertices.size() - 1; i++) {
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

	std::println("skybox name: [{}]", skyBoxName);

	const std::string basePath = std::string(ENGINE_SKY_DIR) + skyBoxName;

	const std::string paths[6] = {
		basePath + "rt.tga",
		basePath + "lf.tga",
		basePath + "up.tga",
		basePath + "dn.tga",
		basePath + "ft.tga",
		basePath + "bk.tga",
	};

	skybox = Skybox(paths, new Shader("assets/shaders/skybox.vert", "assets/shaders/skybox.frag"), SKYBOX_TYPE_GOLDSRC);
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
	glm::vec3 planeNormal = glm::vec3(cPlane.vNormal.x, cPlane.vNormal.z, -cPlane.vNormal.y);

	float segmentStartDistance = glm::dot(
		segmentStart,
		BSP_TO_ENGINE_VECTOR(cPlane.vNormal)
	) - cPlane.fDist;

	float segmentEndDistance = glm::dot(
		segmentEnd,
		BSP_TO_ENGINE_VECTOR(cPlane.vNormal)
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