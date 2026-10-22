#include <DebugDraw.hpp>
#include <Camera.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Log.hpp>
#include <Shader.hpp>

// debug library
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <SDL3/SDL.h>

namespace DebugDraw {

    #define DEBUG_DRAWER_BUFFER_SIZE 512

    bool drawingIsEnabled = false;
    bool debugMenuIsEnabled = false;

    Camera* camera = nullptr;

    GLuint vao = 0;
    GLuint vbo = 0;

    Shader* AABBShader = nullptr;
    Shader* gizmoShader = nullptr;
    Shader* pointShader = nullptr;

    std::vector<impl::DebugDrawCommand> commands{};

    std::vector<std::pair<std::string, std::string>> menuInfoItems{};

    namespace impl {

        void _boundingBoxInternal(const AABB& box, const glm::vec3& color) {

            glm::vec3 min = box.min;
            glm::vec3 max = box.max;

            glm::vec3 boxLines[] = {
                // bottom square
                {min.x,min.y,min.z}, {max.x,min.y,min.z},
                {max.x,min.y,min.z}, {max.x,min.y,max.z},
                {max.x,min.y,max.z}, {min.x,min.y,max.z},
                {min.x,min.y,max.z}, {min.x,min.y,min.z},
                // top square
                {min.x,max.y,min.z}, {max.x,max.y,min.z},
                {max.x,max.y,min.z}, {max.x,max.y,max.z},
                {max.x,max.y,max.z}, {min.x,max.y,max.z},
                {min.x,max.y,max.z}, {min.x,max.y,min.z},
                // vertical edges
                {min.x,min.y,min.z}, {min.x,max.y,min.z},
                {max.x,min.y,min.z}, {max.x,max.y,min.z},
                {max.x,min.y,max.z}, {max.x,max.y,max.z},
                {min.x,min.y,max.z}, {min.x,max.y,max.z},
            };

            // insert box data to buffer
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(boxLines), boxLines);

            // draw
            AABBShader->bind();
            AABBShader->setUniformMat4("u_view", camera->getViewMatrix());
            AABBShader->setUniformMat4("u_projection", camera->getProjectionMatrix());
            AABBShader->setUniformVec3("u_color", color);
            glDrawArrays(GL_LINES, 0, 24);

            // unbind vao
            glBindVertexArray(0);
        }

        void _pointInternal(const glm::vec3& point, const glm::vec3& color, float size) {

            glm::vec3 lines[] = {
                point - glm::vec3(size, 0, 0), point + glm::vec3(size, 0, 0),
                point - glm::vec3(0, size, 0), point + glm::vec3(0, size, 0),
                point - glm::vec3(0, 0, size), point + glm::vec3(0, 0, size),
            };

            // dont vanish behind geometry
            //glDisable(GL_DEPTH_TEST);

            // insert line data
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lines), lines);

            // draw
            pointShader->bind();
            pointShader->setUniformMat4("u_view", camera->getViewMatrix());
            pointShader->setUniformMat4("u_projection", camera->getProjectionMatrix());
            pointShader->setUniformMat4("u_model", glm::mat4(1.0f));
            pointShader->setUniformVec3("u_color", color);
            glDrawArrays(GL_LINES, 0, 6);

            // unbind vao and re-enable z testing
            //glEnable(GL_DEPTH_TEST);
            glBindVertexArray(0);
        }

        void _orientationGizmoInternal() {

            // dont vanish behind geometry
            glDisable(GL_DEPTH_TEST);

            glm::vec3 axisLines[] = {
                {0,0,0}, {1,0,0},
                {0,0,0}, {0,1,0},
                {0,0,0}, {0,0,1}
            };

            // set the 3d gizmos position infront of the camera by a bit
            glm::vec3 gizmoPos = camera->getPosition() + camera->getForwardDirection() * 2.0f;
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), gizmoPos);

            // scale down gizmo a bit
            modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f));

            // insert gizmo data
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(axisLines), axisLines);

            // draw
            gizmoShader->bind();
            gizmoShader->setUniformMat4("u_view", camera->getViewMatrix());
            gizmoShader->setUniformMat4("u_projection", camera->getProjectionMatrix());
            gizmoShader->setUniformMat4("u_model", modelMatrix);
            glDrawArrays(GL_LINES, 0, 6);

            // unbind vao and re-enable z testing
            glEnable(GL_DEPTH_TEST);
            glBindVertexArray(0);
        }

        void draw(DebugBoundingBox boundingBox) {
            _boundingBoxInternal(boundingBox.aabb, boundingBox.color);
        }

        void draw(DebugPoint point) {
            _pointInternal(point.position, point.color, point.size);
        }

        void draw(DebugGizmo) {
            _orientationGizmoInternal();
        }

        void _addMenuInfoItem(const std::string& title, const std::string& value) {
            std::pair infoItem = {title, value};
            menuInfoItems.emplace_back(infoItem);
        }

        void _debugMenuDraw() {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowSize(ImVec2(0, 0));

            ImGui::Begin("Debug");

            for (auto& [title, value] : menuInfoItems) {
                std::string infoString = std::format("{}: {}", title, value);
                ImGui::Text("%s", infoString.c_str());
            }

            ImGui::End();

            ImGui::Render();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

    }

    void init(const InitContext& initContext) {

        camera = initContext.baseCamera;

        AABBShader = new Shader("debugAABB", "assets/shaders/debug/debugAABB.vert", "assets/shaders/debug/debugAABB.frag");
        gizmoShader = new Shader("debug3DGizmo", "assets/shaders/debug/debug3DGizmo.vert", "assets/shaders/debug/debug3DGizmo.frag");
        pointShader = AABBShader;

        commands.clear();

        // create vertex buffer and array
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        // bind array and buffer to add data and attributes
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // set an empty buffer for all draw ops
        glBufferData(GL_ARRAY_BUFFER, DEBUG_DRAWER_BUFFER_SIZE * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

        // add attribute array
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

        glBindVertexArray(0);

        // create imgui context and bind to sdl3 window
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        style.ScaleAllSizes(main_scale);        // bake a fixed style scale
        style.FontScaleDpi = main_scale;        // set initial font scale

        ImGui_ImplSDL3_InitForOpenGL(initContext.window, initContext.glContext);
        ImGui_ImplOpenGL3_Init("#version 460");

        commands.clear();
        menuInfoItems.clear();

        setEnabled(false);
    }

    void cleanup() {
        camera = nullptr;
        delete AABBShader;
        delete gizmoShader;
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        commands.clear();
        // just in case
        setEnabled(false);
    }

    void setEnabled(bool enabled) {
        drawingIsEnabled = enabled;
    }

    void setDebugMenuEnabled(bool enabled) {
        debugMenuIsEnabled = enabled;
    }

    void flush() {

        if (!drawingIsEnabled) {
            commands.clear();
            return;
        }

        if (camera == nullptr) {
            Log::error(Log::Channel::RENDERER, "DebugDrawer camera is null. Flushing debug draw queue.");
            commands.clear();
            return;
        }

        for (const impl::DebugDrawCommand& command : commands)
            std::visit([](const auto& cmd) { draw(cmd); }, command);

        if (debugMenuIsEnabled) {
            impl::_debugMenuDraw();
        }

        commands.clear();
        menuInfoItems.clear();
    }

    void boundingBox(const glm::vec3& min, const glm::vec3& max, glm::vec3 color) {
        AABB box = {min, max};
        commands.emplace_back(impl::DebugBoundingBox{
            box,
            color
        });
    }

    void boundingBox(const AABB& box, glm::vec3 color) {
        commands.emplace_back(impl::DebugBoundingBox{
           box,
            color
        });
    }

    void point(const glm::vec3& point, const glm::vec3& color, float size) {
        commands.emplace_back(impl::DebugPoint{
            point,
            color,
            size
        });
    }

    void orientationGizmo() {
        commands.emplace_back(impl::DebugGizmo{});
    }
}








