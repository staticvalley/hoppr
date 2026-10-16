#pragma once

#include <glm/glm.hpp>
#include <format>

namespace Conversions {

    template <typename T>
    concept isVec3 = requires(const T& v) {
        { v.x }  -> std::convertible_to<float>;
        { v.y }  -> std::convertible_to<float>;
        { v.z }  -> std::convertible_to<float>;
    };

    /**
     * @brief goldsrc bsp's/model's have +z as up, converts to (x, y, z) opengl format
     * @tparam T any vector struct that has .x, .y, .z members
     * @param v goldsrc oriented vector to convert to engines orientation
     * @return converted engine vector
     */
    template <isVec3 T>
    inline glm::vec3 goldsrcToEngineVec(const T& v) { return glm::vec3((float)v.x, (float)v.z, (float)-v.y); }

    /**
     * @brief goldsrc bsp's/model's have +z as up, converts to (x, y, z) opengl format
     * @param v vector in float array form (x, y, z) == ([0], [1], [2])
     * @return converted engine vector
     */
    inline glm::vec3 goldsrcToEngineVec(const float v[3]) { return glm::vec3(v[0], v[2], -v[1]); }


    /**
     * @brief goldsrc bsp's/model's have +z as up, converts to from (x, y, z) opengl format
     * @tparam T any vector struct that has .x, .y, .z members
     * @param v engine oriented vector to convert to goldsrc's orientation
     * @return converted goldsrc vector
     */
    template <isVec3 T>
    inline glm::vec3 engineToGoldsrcVec(const T& v) { return glm::vec3((float)v.x, (float)-v.z, (float)v.y); }


    /**
     * @brief goldsrc bsp's/model's have +z as up, converts to from (x, y, z) opengl format
     * @param v vector in float array form (x, y, z) == ([0], [1], [2])
     * @return converted goldsrc vector
     */
    inline glm::vec3 engineToGoldsrcVec(const float v[3]) { return glm::vec3(v[0], -v[2], v[1]); }

}

namespace std {
    /**
     * @brief print formatter for any vector type with (.x, .y, .z)
     */
    template <Conversions::isVec3 T>
    struct formatter<T> : formatter<float> {
        auto format(const T& vec, format_context& ctx) const {
            auto out = ctx.out();

            out = std::format_to(out, "[x: ");
            out = formatter<float>::format(vec.x, ctx);

            out = std::format_to(out, ", y: ");
            out = formatter<float>::format(vec.y, ctx);

            out = std::format_to(out, ", z: ");
            out = formatter<float>::format(vec.z, ctx);

            return std::format_to(out, "]");
        }
    };
}