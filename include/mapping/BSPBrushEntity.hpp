#pragma once

#include <BSPFile.hpp>
#include <glm/glm.hpp>

/**
 * @todo Only @code LADDER@endcode has full impl.
 */
enum class BrushEntityType: uint8_t {
    UNKNOWN,
    WALL,
    LADDER,
    DOOR,
    TRIGGER,
};

class BSPBrushEntity {
public:
    BSPBrushEntity(BSPEntity entity);
    ~BSPBrushEntity();

    int modelIndex = -1;
    BrushEntityType type = BrushEntityType::UNKNOWN;
    glm::vec3 origin {-1.0f};
};