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

#define PM_BHOP_MAX_SPEED_MULTIPLIER        1.7f

#define MAX_LADDER_SPEED                    200.0f

/**
 * todo: other movement types could be added later
 */
enum class MovementType : uint8_t {
    WALK   = 0,
    NOCLIP = 1,
};

/**
 * @brief bsp hull trace type
 */
enum class HullType : uint8_t {
    STANDING = 1,
    CROUCHING = 3,
};

struct PlayerMovement {
    /**
     * @brief current position of player
     * @note origin is the center of the player hull
     */
    glm::vec3 origin = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);

    /// raw x movement value
    float forwardMove   = 0.0f;
    /// raw z movement value
    float rightMove     = 0.0f;
    /// raw y movement value
    float upMove        = 0.0f;

    /// camera forward view direction
    glm::vec3 forwardView = glm::vec3(0.0f, 0.0f, -1.0f);
    /// camera right view direction
    glm::vec3 rightView  = glm::vec3(1.0f, 0.0f, 0.0f);

    /// has player pressed the JUMP action bind
    bool jumpPressed    = false;
    /// has player pressed the CROUCH action bind
    bool crouchPressed  = false;

    /// is player currently in a jump
    bool inJump         = false;
    /// is player currently in a duck
    bool isDucking      = false;
    /// is player currently sneaking
    bool isSneaking     = false;
    /// is player currently on the ground
    bool isGrounded     = false;
    /// is player currently in water @todo unimplemented
    bool inWater        = false;

    /// saved offset in the stuck table to try from
    int stuckOffsetIndex = 0;

    MovementType moveType = MovementType::WALK;

    /// hull type player is currently in (ie. standing or crouching)
    HullType hullType = HullType::STANDING;

    /**
     * @brief update movement members in given map
     * @param dt time interval between last frame and current frame
     * @param map currently loaded map to update movement on
     */
    void update(
        float dt,
        BSPMap& map
    );

    /**
     * @brief calculates flattened speed value for hud
     * @return length of velocity(x, z) vector
     */
    float getSpeed();

private:

    /**
     * @brief run through and apply stucktable to origin if player is stuck (mirrors goldsrc's PM_CheckStuck)
     * @param map BSP map to check movement against
     * @return if player is still stuck @code(true == stuck, false == unstuck)@endcode
     */
    bool checkStuck(BSPMap& map);

    /**
     * helper function for checking if a point intersects with solid space
     * @param point position (in hammer units) to check
     * @param map BSP map to check against
     * @return if position is fully within a solid @code(true == all solid, false == valid space)@endcode
     */
    bool testPosition(const glm::vec3& point, BSPMap& map);

    /**
     * @brief generate stuck table for nudging/origin offsets (mirrors goldsrc's PM_CreateStuckTable)
     * @return the stucktable
     * @note just copied from the pm_shared version. read that over for more info
     */
    static const std::array<glm::vec3, 54>& createStuckTable();

    /**
     * @brief calculate movement when on ground and @code MovementType == WALK@endcode (mirrors goldsrc's PM_WalkMove)
     * @param dt time interval between last frame and current frame
     * @param wishDir desired movement vector
     * @param map BSP map to check movement against
     */
    void walkMove(float dt, const glm::vec3& wishDir, BSPMap& map);

    /**
     * @brief calculate origin when @code MovementType == NOCLIP@endcode
     * @param dt time interval between last frame and current frame
     * @param wishDir desired movement vector
     */
    void noclipMove(float dt, const glm::vec3& wishDir);

    /**
     * @brief calculate movement when in air and @code MovementType == WALK@endcode (mirrors goldsrc's PM_FlyMove)
     * @param dt time interval between last frame and current frame
     * @param map BSP map to check movement against
     * @return blockage code incase movement is blocked
     * @note right now, blockage codes dont really mean anything or do anything, the blockage is dealt with directly in the function
     */
    int flyMove(float dt, BSPMap& map);

    /**
     * @brief handles movement when crouching and checking if player can un-crouch
     * @param map BSP map to check movement against
     */
    void duck(BSPMap& map);

    /**
     * @brief applies acceleration to velocity (mirrors goldsrc's PM_Accelerate)
     * @param wishDir desired movement vector
     * @param wishSpeed length of desired movement vector * `PM_MOVEMENT_SPEED`
     * @param accel base acceleration
     * @param dt time interval between last frame and current frame
     */
    void accelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt);


    /**
     * @brief applies air acceleration to velocity (mirrors goldsrc's PM_AirAccelerate)
     * @param wishDir desired movement vector
     * @param wishSpeed length of desired movement vector * `PM_MOVEMENT_SPEED`
     * @param accel base acceleration
     * @param dt time interval between last frame and current frame
     */
    void airAccelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt);

    /**
     * @brief apply friction to velocity vector (mirrors goldsrc's PM_Friction)
     * @param dt time interval between last frame and current frame
     */
    void applyFriction(float dt, BSPMap& map);

    /**
     * @brief recalculate velocity vector based on impacted normal (mirrors goldsrc's PM_ClipVelocity)
     * @param in vector to recalculate bounce for
     * @param normal impact surface normal
     * @param out redirected "in" vector
     * @param overBounce scalar factor to add/remove slide in the "in" vector's direction
     * @return blocked flags @code(0x01 == Floor, 0x02 == Wall)@endcode
     */
    int  clipVelocity(const glm::vec3& in, const glm::vec3& normal, glm::vec3& out, float overBounce);

    /**
     * @brief check if grounded (mirrors goldsrc's PM_CatagorizePosition)
     * @param map BSP map to check movement against
     */
    void categorizePosition(BSPMap& map);

    /**
     * @brief guard for extreme velocity values (mirrors goldsrc's PM_CheckVelocity)
     */
    void checkVelocity();

    /**
     * @brief crop velocity values, correcting bhopping (mirrors goldsrc's PM_PreventMegaBunnyJumping)
     */
    void preventMegaBunnyJump();

    /**
     * @brief check if current player origin intersects with any ladder bounding boxes on map
     * @param map BSP map to check for ladders
     * @return intersected ladder brush entity object if found, @code nullptr@endcode otherwise.
     */
    const BSPBrushEntity* findLadder(BSPMap& map);

    /**
     * @brief perform movement on ladders
     * @note uses raw view/movement values based on player orientation on ladder
     * @param ladder ladder player is interacting with
     * @param map BSP map to check movement against
     * @return if the player is still interacting with the ladder @code(true == mounted, false == not mounted)@endcode
     */
    bool ladderMove(const BSPBrushEntity& ladder, BSPMap& map);

    /**
     * @brief get normal of ladder surface in the direction the player is currently facing
     * @param ladderModel ladder player is interacting with
     * @param map BSP map to check movement against
     * @return normal of ladder surface player is mounted on
     */
    glm::vec3 findLadderNormal(const BSPModel& ladderModel, BSPMap& map);

    /**
     * @brief BSP map hull trace wrapper (just for cleaner callsites)
     * @param start start point of trace
     * @param end end point of trace
     * @param map map to trace through
     * @param hullType hull level to trace the map through
     * @return trace results
     */
    BSPHullTrace traceHull(const glm::vec3& start, const glm::vec3& end, BSPMap& map, HullType hullType);
};