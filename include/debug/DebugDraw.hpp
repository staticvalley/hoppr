#pragma once
#include <Shader.hpp>
#include <Collision.hpp>
#include <Camera.hpp>
#include <variant>

/**
 * @brief global singleton for debugging visually in 3D space
 * @note uses a command queue for drawing, which must be flushed
 */
namespace DebugDraw {

    namespace impl {

        struct DebugPoint {
            glm::vec3 position;
            glm::vec3 color;
            float size;
        };

        struct DebugBoundingBox {
            AABB aabb;
            glm::vec3 color;
        };

        struct DebugGizmo {};

        using DebugDrawCommand = std::variant<DebugPoint, DebugBoundingBox, DebugGizmo>;

    }

    /**
     * @brief set up and initialize draw buffer and vao
     * @param debugCamera camera that debug items will draw too
     * @note initialization disabled by default (to enable, call setEnabled(false);)
     */
    void init(Camera* debugCamera);

    /**
     * @brief sets debug drawing as enabled
     * @param enabled true = on, false = off
     */
    void setEnabled(bool enabled);

    /**
     * @brief deallocate draw buffer and vao
     */
    void cleanup();

    /**
     * @brief flushes drawing queue (performs draw calls of all debug objects)
     */
    void flush();

    /**
     * @brief queues axis-aligned bounding box wireframe to be drawn
     * @param box axis-aligned bounding box
     * @param color rendered wifeframe color
     */
    void boundingBox(const AABB& box, glm::vec3 color = glm::vec3(0, 1, 0));


    /**
     * @brief queues axis-aligned bounding box wireframe to be drawn
     * @param min min point of bounding box
     * @param max max point of bounding box
     * @param color rendered wifeframe color
     */
    void boundingBox(const glm::vec3& min, const glm::vec3& max, glm::vec3 color = glm::vec3(0, 1, 0));

    /**
     * @brief queues 3D axis orientation gizmo to be drawn
     */
    void orientationGizmo();

    /**
     * @brief queues axis-aligned star in world space at point to be drawn
     * @param point origin of rendered point in world space
     * @param color rendered point color
     * @param size point size scaler
     */
    void point(const glm::vec3& point, const glm::vec3& color, float size = 0.5f);

}