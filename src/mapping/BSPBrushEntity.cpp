#include <BSPBrushEntity.hpp>
#include <Log.hpp>
#include <Util.hpp>

BSPBrushEntity::BSPBrushEntity(BSPEntity entity) {
    const auto parseModelIndex = [](std::string& s) {
        return std::stoi(s.substr(1));
    };

    const auto parseOrigin = [](std::string& s) {
        glm::vec3 v(0.0f);
        std::istringstream stringStream(s);
        stringStream >> v.x >> v.y >> v.z;
        return v;
    };

    const auto getType =  [](std::string& className) {
        if (className == "func_ladder") return BrushEntityType::LADDER;
        if (className == "func_door") return BrushEntityType::DOOR;
        return BrushEntityType::UNKNOWN;
    };

    if (entity.contains("classname")) {
        // get brush type
        type = getType(entity.at("classname"));
        if (entity.contains("model")) {
            try {
                modelIndex = parseModelIndex(entity.at("model"));
            } catch (const std::invalid_argument& e) {
                // format isn't correct o algo
                Log::error(Log::Channel::MAPPING, "brush entity '{}''s 'model' key is invalid. Found string: '{}'", entity.at("classname"), entity.at("model"));
            }
        } else {
            if (type != BrushEntityType::UNKNOWN) {
                Log::warn(Log::Channel::MAPPING, "brush entity '{}' has no 'model' key", entity.at("classname"));
            }
        }

        if (entity.contains("origin")) {
            origin = parseOrigin(entity.at("origin"));
            //Log::debug(Log::Channel::MAPPING, "brush entity '{}' has a special dynamic origin: {}", entity.at("classname"), origin);
        } else {
            origin = glm::vec3(0.0f);
        }
    }
}

BSPBrushEntity::~BSPBrushEntity() {};