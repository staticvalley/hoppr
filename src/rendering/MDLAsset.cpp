#include <MDLAsset.hpp>
#include <MDLFile.hpp>

#include <Log.hpp>
#include <Util.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>


namespace {
    /**
     * @brief reads entire file into resizable buffer
     * @param path path string to file to read
     * @param outBuffer resizable buffer to fill with file contents
     * @return success @code (true == success, false == file not found or error)
     * @todo c++ style casts?
     */
    bool readFileToByteBuffer(const std::string& path, std::vector<uint8_t>& outBuffer) {
        // open file in binary mode, skip to end
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        // file not open
        if (!file) return false;

        outBuffer.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read((char*)outBuffer.data(), (std::streamsize)outBuffer.size());
        return (bool)(file);
    }

    /**
     * @brief gets a byte chunk at some given offset in a buffer
     * @tparam T type of byte chunk
     * @param buffer buffer to be read from
     * @param offset offset to byte chunk
     * @param count how many chunks to read
     * @return reference to byte chunk in buffer
     */
    template<typename T>
    const T* referenceAtOffset(const std::vector<uint8_t>& buffer, const int32_t offset, const int32_t count = 1) {
        // bounds check
        if (offset < 0 || count < 0) return nullptr;

        const size_t end = (size_t)offset + (size_t)count * sizeof(T);

        // overflow check
        if (end > buffer.size()) return nullptr;

        return (const T*)(buffer.data() + offset);
    }

    /**
     * @brief reads .mdl file header into buffer
     * @param buffer buffer to read header into
     * @param path path to .mdl file
     * @return .mdl header object
     */
    const MDLHeader* readHeader(const std::vector<uint8_t>& buffer, const std::string& path) {
        const MDLHeader* header = referenceAtOffset<MDLHeader>(buffer, 0);

        if (!header) {
            Log::warn(Log::Channel::MDL, "header of file '{}' could not be read", path);
            return nullptr;
        }

        if (std::memcmp(&(header->id), MDL_MAGIC, MDL_MAGIC_SIZE) != 0) {
            Log::warn(Log::Channel::MDL, "file '{}' found with incorrect magic (found {})", path, header->id);
            return nullptr;
        }

        if (header->version != MDL_VERSION) {
            Log::warn(Log::Channel::MDL, "file '{}' is not a v10 studio model (found {})", path, header->version);
            return nullptr;
        }

        return header;
    }

    /**
     * @brief creates world matrix compiling all local matrices of models bones
     * @note model bone chunks start at top node and work down to all children
     * @param bones reference to bone chunk
     * @param count total bone count
     * @return model world matrix
     */
    std::vector<glm::mat4> buildBindPose(const MDLBone* bones, int count) {
        std::vector<glm::mat4> world(count);

        for (int i = 0; i < count; i++) {
            const MDLBone& bone = bones[i];

            const glm::vec3 localBonePosition = glm::vec3(bone.value[0], bone.value[1], bone.value[2]);
            const glm::quat localBoneRotation = glm::vec3(bone.value[3], bone.value[4], bone.value[5]);

            // create local matrix
            glm::mat4 local = glm::translate(glm::mat4(1.0f), localBonePosition);

            // add quat rotation
            local *= glm::mat4_cast(glm::quat(localBoneRotation));

            const bool hasParent = bone.parent >= 0 && bone.parent < i;

            // chain parents local transform matrix with local matrix
            world[i] = hasParent ? world[bone.parent] * local : local;
        }

        return world;
    }

    /**
     * @brief builds engine textures given .mdl texture references
     * @param buffer .mdl file buffer with texture data
     * @param texture reference to single texture chunk to compile into engine texture
     * @return paletted engine texture
     */
    std::shared_ptr<Texture> buildTexture(const std::vector<uint8_t>& buffer, const MDLTexture& texture) {
        if (texture.width <= 0 || texture.height <= 0 || texture.width > 8192 || texture.height > 8192) {
            Log::error(Log::Channel::MDL, "texture '{}' has invalid size {}x{}", texture.name, texture.width, texture.height);
            return nullptr;
        }

        const int32_t pixelCount = texture.width * texture.height;
        const uint8_t* indices = referenceAtOffset<uint8_t>(buffer, texture.index, pixelCount);
        const uint8_t* palette = referenceAtOffset<uint8_t>(buffer, texture.index + pixelCount, 256 * 3);

        if (!indices || !palette) {
            Log::error(Log::Channel::MDL, "texture '{}' pixel data is out of bounds", texture.name);
            return nullptr;
        }

        const bool isTransparent = (texture.flags & MDL_ALPHATEST) != 0;
        if (isTransparent) Log::debug(Log::Channel::MDL, "texture '{}' has transparent textures", texture.name);

        return std::make_shared<Texture>(indices, palette, texture.width, texture.height, isTransparent);
    }
}

/**
 * @brief loads in .mdl file as an asset
 * @param path path to .mdl file
 * @return .mdl asset with textures
 */
std::unique_ptr<MDLAsset> loadMDL(const char* path) {
    // read file to byte buffer
    std::vector<uint8_t> fileBuffer;
    if (!readFileToByteBuffer(path, fileBuffer)) {
        Log::error(Log::Channel::MDL, "could not read model file '{}'", path);
        return nullptr;
    }

    // read header
    const MDLHeader* header = readHeader(fileBuffer, path);
    if (!header) return nullptr;

    // when a model has no textures they are found in "(name)T.mdl"
    // set the texture buffer to be the original .mdl file incase the textures are stored within the same file
    std::vector<uint8_t> textureFileBuffer;
    const std::vector<uint8_t>* textureBuffer = &fileBuffer;
    const MDLHeader* textureHeader = header;

    // check if "(name)T.mdl" exists if current .mdl file has no textures
    if (header->numTextures == 0) {
        std::filesystem::path texturePath(path);
        texturePath.replace_filename(texturePath.stem().string() + "T.mdl");
        Log::debug(Log::Channel::MDL, "'{}' references external textures in '{}'", path, texturePath.string());

        if (readFileToByteBuffer(texturePath.string(), textureFileBuffer)) {
            if (const MDLHeader* h = readHeader(textureFileBuffer, texturePath.string())) {
                textureBuffer = &textureFileBuffer;
                textureHeader = h;
            }
        } else {
            Log::warn(
                Log::Channel::MDL,
                "'{}' has no textures and '{}' was not found",
                path,
                texturePath.string()
            );
        }
    }

    // get bones & build bind pose
    const MDLBone* bones = referenceAtOffset<MDLBone>(fileBuffer, header->boneIndex, header->numBones);
    if (!bones || header->numBones <= 0) {
        Log::error(Log::Channel::RENDERER, "'{}' has no valid bone table", path);
        return nullptr;
    }
    // collection of all bone local transformation matrices
    const std::vector<glm::mat4> worldBoneMatrices = buildBindPose(bones, header->numBones);

    // current name of model is just the filename
    std::unique_ptr<MDLAsset> asset = std::make_unique<MDLAsset>(MDLAsset {
        .model = Model(header->name),
    });

    // compile all textures and store inside the asset struct
    const MDLTexture* textureInfo = referenceAtOffset<MDLTexture>(
        *textureBuffer,
        textureHeader->textureIndex,
        textureHeader->numTextures
    );
    size_t textureCount = textureInfo ? textureHeader->numTextures: 0;
    std::vector<std::shared_ptr<Texture>> texturePointers(textureCount,nullptr);

    // build all textures
    for (size_t i = 0; i < textureCount; i++) {
        std::shared_ptr<Texture> texture = buildTexture(*textureBuffer, textureInfo[i]);
        texturePointers[i] = texture;
        // store inside asset struct
        if (texture) asset->textures.push_back(texture);
    }

    // map meshes skin references to texture indices
    // reference start of skin chunk
    const int16_t* skinTable = referenceAtOffset<int16_t>(
        *textureBuffer,
        textureHeader->skinIndex,
        textureHeader->numSkinRef
    );

    // reference start of body part chunk
    const MDLBodyPart* bodyParts = referenceAtOffset<MDLBodyPart>(
        fileBuffer,
        header->bodyPartIndex,
        header->numBodyParts
    );

    if (!bodyParts) {
        Log::error(Log::Channel::MDL, "'{}' has no valid body part table", path);
        return nullptr;
    }

    for (int32_t bp = 0; bp < header->numBodyParts; bp++) {

        // no submodels
        if (bodyParts[bp].numModels < 1) continue;

        // reference submodel of bodypart
        const MDLSubModel* subModel = referenceAtOffset<MDLSubModel>(fileBuffer, bodyParts[bp].modelIndex, 1);
        if (!subModel) continue;

        const float* positions = referenceAtOffset<float>(fileBuffer, subModel->vertIndex, subModel->numVerts * 3);
        const uint8_t* vertBones = referenceAtOffset<uint8_t>(fileBuffer, subModel->vertInfoIndex, subModel->numVerts);
        const float* normals = referenceAtOffset<float>(fileBuffer, subModel->normIndex, subModel->numNorms * 3);
        const uint8_t* normBones = referenceAtOffset<uint8_t>(fileBuffer, subModel->normInfoIndex, subModel->numNorms);
        const MDLMesh* meshes = referenceAtOffset<MDLMesh>(fileBuffer, subModel->meshIndex, subModel->numMesh);

        // no meshes for submodel (invisible), skip
        if (subModel->numMesh == 0) continue;

        if (!positions || !vertBones || !normals || !normBones || !meshes) {
            Log::error(Log::Channel::MDL, "'{}' body part {} has corrupt submodel data, skipping", path, bp);
            continue;
        }

        // gets local translation matrix for given bone
        const auto boneMatrix = [&](uint8_t bone) -> const glm::mat4& {
            return worldBoneMatrices[std::min<size_t>(bone, worldBoneMatrices.size() - 1)];
        };

        for (int32_t m = 0; m < subModel->numMesh; m++) {
            const MDLMesh& mesh = meshes[m];

            // get texture for current mesh in submodel (with bounds checks)
            int textureIndex = -1;
            if (skinTable && mesh.skinRef >= 0 && mesh.skinRef < textureHeader->numSkinRef)
                textureIndex = skinTable[mesh.skinRef];
            if (textureIndex < 0 || textureIndex >= (int)(texturePointers.size()))
                textureIndex = -1;

            float textureWidth = 1.0f;
            float textureHeight = 1.0f;
            if (textureIndex >= 0) {
                textureWidth = (float)textureInfo[textureIndex].width;
                textureHeight = (float)textureInfo[textureIndex].height;
            }

            /*
             * https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#mesh
             *
             * from the assimp wiki:
             *
             * "Half-Life models store mesh triangles as strips or fans. Each mesh chunk specify a list of triverts.
             * This list is divided in sequences and each new sequence means a different triangle type. Each sequence
             * starts with the number of triverts N used in the sequence. If N is positive, this means that the
             * sequence is a triangle strip, otherwise it is a triangle fan."
             */
            // triangle commands: 0 ends the mesh, n > 0 is a strip of n verts, n < 0 is a fan of -n verts.
            // each vert is 4 shorts: [position index, normal index, s, t]
            // bounds check trivert index @note maybe log in future
            if (mesh.triIndex < 0 || (size_t)(mesh.triIndex) >= fileBuffer.size()) continue;

            // first trivert elem in sequence is triangle type (strip or fan)
            const int16_t* sequence = (const int16_t*)(fileBuffer.data() + mesh.triIndex);
            const int16_t* sequenceEnd = (const int16_t*)(fileBuffer.data() + fileBuffer.size());

            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;

            // goldsrc studio triangles are clockwise, swap two corners so they're counter-clockwise
            const auto addTriangle = [&](GLuint a, GLuint b, GLuint c) {
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(b);
            };

            bool corrupt = false;

            while (!corrupt && sequence < sequenceEnd) {

                // end of mesh sequence (ie. next sequence has 0 triverts)
                if (*sequence == 0) break;

                // n is positive = fan, n is negative = strip
                const bool isFan = *sequence < 0;

                // how many triverts are there to read for strip/fan
                const uint16_t sequenceCount = std::abs(*sequence);

                // skip 'n' sequence header
                sequence++;

                constexpr int16_t trivertShortWidth = sizeof(MDLTrivert) / sizeof(int16_t);

                // bounds check for full fan/strip sequence
                if (sequenceCount > (sequenceEnd - sequence) / trivertShortWidth) {
                    corrupt = true;
                    break;
                }

                const GLuint first = (GLuint)(vertices.size());

                for (int t = 0; t < sequenceCount; t++) {
                    const MDLTrivert* trivert = (const MDLTrivert*)(sequence + t * trivertShortWidth);

                    // bounds check for vertex and normal indices
                    if ((trivert->vertIndex < 0 || trivert->vertIndex >= subModel->numVerts) ||
                        (trivert->normIndex < 0 || trivert->normIndex >= subModel->numNorms)) {
                        corrupt = true;
                        break;
                    }

                    const glm::vec3 position(
                        positions[trivert->vertIndex * 3],
                        positions[trivert->vertIndex * 3 + 1],
                        positions[trivert->vertIndex * 3 + 2]
                    );

                    const glm::vec3 normal(
                        normals[trivert->normIndex * 3],
                        normals[trivert->normIndex * 3 + 1],
                        normals[trivert->normIndex * 3 + 2]
                    );

                    // translate to world
                    const glm::vec3 worldPosition = glm::vec3(boneMatrix(vertBones[trivert->vertIndex]) * glm::vec4(position, 1.0f));
                    const glm::vec3 worldNormal = glm::mat3(boneMatrix(normBones[trivert->normIndex])) * normal;

                    Vertex vertex;
                    vertex.position = Conversions::goldsrcToEngineVec(worldPosition);
                    vertex.normal = Conversions::goldsrcToEngineVec(worldNormal);
                    // normalization based on texture since (s,t) are absolute per spec
                    vertex.uv = glm::vec2(trivert->s / textureWidth, trivert->t / textureHeight);
                    vertices.push_back(vertex);
                }

                if (corrupt) break;

                if (isFan) {
                    // fan winding from first point
                    for (int k = 1; k + 1 < sequenceCount; k++)
                        addTriangle(first, first + k, first + k + 1);
                } else {
                    // strip winding (zig-zag)
                    for (int k = 0; k + 2 < sequenceCount; k++) {
                        // strips alternate winding every triangle
                        if (k % 2 == 0) addTriangle(first + k, first + k + 1, first + k + 2);
                        else addTriangle(first + k + 1, first + k, first + k + 2);
                    }
                }

                // advance to next sequence
                sequence += sequenceCount * trivertShortWidth;

            }

            if (corrupt)
                Log::warn(
                    Log::Channel::MDL,
                    "'{}' body part {} mesh {} has corrupt triangle data, keeping what was read",
                    path,
                    bp,
                    m
                );

            if (indices.empty()) continue;

            Mesh outMesh(std::move(vertices), std::move(indices));
            outMesh.material.diffuseTexture = textureIndex >= 0 ? texturePointers[textureIndex] : nullptr;
            asset->model.addMesh(std::move(outMesh));
        }
    }

    asset->model.updateBoundingBox();

    Log::info(
        Log::Channel::MDL,
        "loaded model '{}' ({} bones, {} textures)",
        path,
        header->numBones,
        asset->textures.size()
    );


    return asset;
}
