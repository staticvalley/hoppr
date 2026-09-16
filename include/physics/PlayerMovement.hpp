#pragma once
#include <BSPMap.hpp>
#include <glm/glm.hpp>

#define PM_MAX_VELOCITY                     2000.0f
#define PM_STOP_EPSILON                     0.1f
#define PM_MAX_CLIP_PLANES                  5
#define PM_BUMP_COUNT                       4

#define PM_MOVEMENT_SPEED                   (250.0f)
#define PM_ACCELERATE                       10.0f
#define PM_AIR_ACCELERATE                   20.0f
#define PM_FRICTION                         6.0f // old friction was 4
#define PM_JUMP_FORCE                       268.3f
#define PM_GRAVITY                          800.0f
#define PM_STEPSIZE                         18.1f
#define PM_STOP_SPEED                       100.0f
#define PM_EDGE_FRICTION                    2.0f
#define PM_GROUNDCHECK_EPSILON              2.0f

#define PM_PLAYER_HEIGHT                    72.0f
#define PM_PLAYER_HEIGHT_CROUCHED           36.0f

#define PM_PLAYER_ORIGIN                    (PM_PLAYER_HEIGHT / 2.0f)
#define PM_PLAYER_ORIGIN_CROUCHED           (PM_PLAYER_HEIGHT_CROUCHED / 2.0f)

#define PM_EYEHEIGHT                        64.0f
#define PM_EYEHEIGHT_CROUCHED               28.0f
#define PM_EYEHEIGHT_FROM_ORIGIN            (PM_EYEHEIGHT - PM_PLAYER_ORIGIN)
#define PM_EYEHEIGHT_FROM_ORIGIN_CROUCHED   (PM_EYEHEIGHT_CROUCHED - PM_PLAYER_ORIGIN_CROUCHED)

#define PM_DUCK_SPEED_MULTIPLIER            0.333f
#define PM_SNEAK_SPEED_MULTIPLIER           0.5f

#define PM_MAXSPEED                         600.0f

#define CLIPPING_EPSILON 0.03125f

enum class MovementType : uint8_t {
    WALK   = 0,
    NOCLIP = 1,
};

enum class HullType : uint8_t {
    STANDING = 1,
    CROUCHING = 3,
};

struct PlayerMovement {
    glm::vec3 origin;
    glm::vec3 velocity;

    bool jumpPressed   = false;
    bool crouchPressed = false;

    bool isDucking     = false;
    bool isSneaking    = false;
    bool isGrounded    = false;
    bool inWater       = false;

    // saved offset in stucktable to try from
    int stuckOffsetIndex = 0;

    MovementType moveType = MovementType::WALK;
    HullType hullType = HullType::STANDING;

    void update(float dt, const glm::vec3& wishDir, BSPMap& map);

private:

    // mirrors PM_CheckStuck
    bool checkStuck(BSPMap& map);

    // mirrors PM_CreateStuckTable
    static const std::array<glm::vec3, 54>& createStuckTable();

    // mirrors PM_WalkMove
    void walkMove(float dt, const glm::vec3& wishDir, BSPMap& map);
    // mirrors PM_NoClip
    void noclipMove(float dt, const glm::vec3& wishDir);
    // mirrors PM_FlyMove � core bump + slide loop
    int  flyMove(float dt, BSPMap& map);

    // mirrors PM_Duck & PM_UnDuck
    void duck(BSPMap& map);

    /**
     * @brief applys acceleration to velocity (mirrors goldsrc's `PM_Accelerate`)
     * @param wishDir desired movement vector
     * @param wishSpeed length of desired movement vector * `PM_MOVEMENT_SPEED`
     * @param accel base acceleration
     * @param dt time interval between last frame and current frame
     */
    void accelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt);
    void airAccelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt);

    /**
     * @brief apply friction to velocity vector (mirrors goldsrc's `PM_Friction`)
     * @param dt ime interval between last frame and current frame
     */
    void applyFriction(float dt, BSPMap& map);

    // mirrors PM_ClipVelocity
    int  clipVelocity(const glm::vec3& in, const glm::vec3& normal, glm::vec3& out, float overBounce);

    // mirrors PM_CatagorizePosition
    void categorizePosition(BSPMap& map);

    // mirrors PM_CheckVelocity
    void checkVelocity();

    BSPHullTrace traceHull(const glm::vec3& start, const glm::vec3& end, BSPMap& map, HullType hullType);
};