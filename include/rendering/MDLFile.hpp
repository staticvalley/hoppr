#pragma once
#include <cstdint>

#define MDL_MAGIC "IDST"
#define MDL_MAGIC_SIZE 4
#define MDL_VERSION 10

#define MDL_ALPHATEST 0x0040

/**
 * Structures are taken from:
 * https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format
 */

/**
 * @brief header
 *
 * @link https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#header @endlink
 */
struct MDLHeader {
    /// magic word ("IDST")
    int32_t id;
    /// file format version @note must be @code 10@endcode to be parsed
    int32_t version;
    /// model name
    char name[64];
    /// file size (bytes)
    int32_t length;
    /// model eye position
    float eyePosition[3];
    /// model hull min extent
    float min[3];
    /// model hull max extent
    float max[3];
    /// model clipping box min extent
    float bbMin[3];
    /// model clipping box max extent
    float bbMax[3];
    /// general flags
    int32_t flags;
    /// number of bones
    int32_t numBones;
    /// offset of the first bone chunk
    int32_t boneIndex;
    /// number of bone controllers
    int32_t numBoneControllers;
    /// offset of the first bone controller chunk
    int32_t boneControllerIndex;
    /// number of hitboxes
    int32_t numHitboxes;
    /// offset of the hitbox chunk
    int32_t hitboxIndex;
    /// number of sequences
    int32_t numSeq;
    /// offset of the first sequence chunk
    int32_t seqIndex;
    /// number of sequence groups
    int32_t numSeqGroups;
    /// offset of the first sequence group chunk
    int32_t seqGroupIndex;
    /// number of textures
    int32_t numTextures;
    /// offset of the first texture chunk
    int32_t textureIndex;
    /// offset of the pixel data of the first texture chunk
    int32_t textureDataIndex;
    /// the number of replaceable textures
    int32_t numSkinRef;
    /// the number of skin families
    int32_t numSkinFamilies;
    /// the offset of the first skin value
    int32_t skinIndex;
    /// the number of body parts
    int32_t numBodyParts;
    /// the offset of the first body part chunk
    int32_t bodyPartIndex;
    /// the number of attachments
    int32_t numAttachments;
    /// the offset of the first attachment chunk
    int32_t attachmentIndex;
    /// @todo what is this member?
    int32_t soundTable;
    /// @todo what is this member?
    int32_t soundIndex;
    /// @todo what is this member?
    int32_t soundGroups;
    /// @todo what is this member?
    int32_t soundGroupIndex;
    /// the number of nodes in the sequence transition graph
    int32_t numTransitions;
    /// the offset of the first transition value of the sequence transition graph
    int32_t transitionIndex;
};
static_assert(sizeof(MDLHeader) == 244);

/**
 * @brief texture chunk
 *
 * @link https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#texture @endlink
 */
struct MDLTexture {
    /// texture name
    char name[64];
    /// texture flags
    int32_t flags;
    /// width in pixels
    int32_t width;
    /// height in pixels
    int32_t height;
    /// offset of the pixel data
    int32_t index;
};
static_assert(sizeof(MDLTexture) == 80);

/**
 * @brief bone chunk
 *
 * @link https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#bone @endlink
 */
struct MDLBone {
    /// bone name
    char name[32];
    /// the bones parent index
    int32_t parent;
    /// bone flags @note unused
    int32_t flags;
    /**
     * @brief the indices of the bone controller with respect to the motion bone used by this bone controller
     * @note [<0] = no bone controller associated with slot
     */
    int32_t boneController[6];
    /**
     * @brief bones rotation and position
     * @note [0-2] = position [x, y, z]
     * @note [3-5] = rotation [r, p, y]
     */
    float value[6];
    /**
     * @brief bones scale of position/rotation when reading animation value data
     * @note [0-2] = position [x, y, z]
     * @note [3-5] = rotation [r, p, y]
     */
    float scale[6];
};
static_assert(sizeof(MDLBone) == 112);

/**
 * @brief body part chunk
 *
 * @link https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#bodypart @endlink
 */
struct MDLBodyPart {
    /// body part name
    char name[64];
    /// the number of models
    int32_t numModels;
    /// divisor to calculate the index of the model relative to its bodypart
    int32_t base;
    /// the offset of the first model chunk
    int32_t modelIndex;
};
static_assert(sizeof(MDLBodyPart) == 76);

/**
 * @brief model chunk
 *
 * @link https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#model @endlink
 */
struct MDLSubModel {
    /// submodel name
    char name[64];
    /// @note unused
    int32_t type;
    /// @note unused
    float boundingRadius;
    /// number of meshes
    int32_t numMesh;
    /// the offset to the first mesh chunk
    int32_t meshIndex;
    /// the number of vertex positions
    int32_t numVerts;
    /// the offset to the first vertexInfoIndex position @note offset to array of int32_t
    int32_t vertInfoIndex;
    /// the offset to the first index position
    int32_t vertIndex;
    /// the number of vertex normals
    int32_t numNorms;
    /// the offset to the first normInfoIndex value @note offset to array of int32_t
    int32_t normInfoIndex;
    /// the offset to the first vertex normal
    int32_t normIndex;
    /// @note unused
    int32_t numGroups;
    /// @note unused
    int32_t groupIndex;
};
static_assert(sizeof(MDLSubModel) == 112);

/**
 * @brief mesh chunk
 *
 * @link https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#mesh @endlink
 */
struct MDLMesh {
    /// number of triverts
    int32_t numTris;
    /// the offset to the first trivert
    int32_t triIndex;
    /// the texture index
    int32_t skinRef;
    /// @note unused
    int32_t numNorms;
    /// @note unused
    int32_t normIndex;
};
static_assert(sizeof(MDLMesh) == 20);

/**
 * @brief trivert chunk
 *
 * @link https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format#trivert @endlink
 */
struct MDLTrivert {
    /// the index of a vec3 position
    int16_t vertIndex;
    /// the index of a vec3 normal
    int16_t normIndex;
    /// horizontal texture coordinate @note absolute coordinate
    int16_t s;
    /// vertical texture coordinate @note absolute coordinate
    int16_t t;
};
static_assert(sizeof(MDLTrivert) == 8);




