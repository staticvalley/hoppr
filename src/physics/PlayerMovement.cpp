#include <PlayerMovement.hpp>
#include <BSPFile.hpp>
#include <algorithm>
#include <print>

// mirrors PM_ClipVelocity
// returns 0x01 if floor, 0x02 if wall
int PlayerMovement::clipVelocity(const glm::vec3& in, const glm::vec3& normal, glm::vec3& out, float overBounce) {
    float backoff = glm::dot(in, normal) * overBounce;
    out = in - normal * backoff;
    // zero out tiny components to avoid oscillation
    for (int i = 0; i < 3; i++) {
        if (out[i] > -PM_STOP_EPSILON && out[i] < PM_STOP_EPSILON)
            out[i] = 0.0f;
    }
    int blocked = 0;
    if (normal.y > 0.0f)  blocked |= 0x01; // floor
    if (normal.y == 0.0f) blocked |= 0x02; // wall

    return blocked;
}

void PlayerMovement::accelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt) {

    // if we are changing directions a bit, slow currentSpeed
    float currentSpeed = glm::dot(velocity, wishDir);
    
    // wishspeed is reduced by amount we are changing directions (sharp turns make you slower)
    float addSpeed = wishSpeed - currentSpeed;

    // dont add any speed
    if (addSpeed <= 0.0f) return;

    // determine amount of acceleration
    float accelSpeed = accel * dt * wishSpeed;
    
    // cap at addspeed
    if (accelSpeed > addSpeed) accelSpeed = addSpeed;

    // adjust velocity based on acceleration speed
    velocity += wishDir * accelSpeed;

}

void PlayerMovement::airAccelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt) {

    // cap wish speed if in air
    float cappedWishSpeed = wishSpeed > 30.0f ? 30.0f : wishSpeed;

    // if we are changing directions a bit, slow currentSpeed
    float currentSpeed = glm::dot(velocity, wishDir);

    // wishspeed is reduced by amount we are changing directions (sharp turns make you slower)
    float addSpeed = cappedWishSpeed - currentSpeed;

    // dont add any speed
    if (addSpeed <= 0.0f) return;

    // determine amount of acceleration
    float accelSpeed = accel * dt * wishSpeed;

    // cap at addspeed
    if (accelSpeed > addSpeed) accelSpeed = addSpeed;

    // adjust velocity based on acceleration speed
    velocity += wishDir * accelSpeed;

}

void PlayerMovement::checkVelocity() {
    for (int i = 0; i < 3; i++) {
        if (std::isnan(velocity[i]))
            velocity[i] = 0.0f;
        velocity[i] = glm::clamp(velocity[i], -PM_MAX_VELOCITY, PM_MAX_VELOCITY);
    }
}

void PlayerMovement::applyFriction(float dt, BSPMap& map) {

    glm::vec3 base_velocity = velocity;

    // get 3d speed
    float speed = glm::length(base_velocity);

    // if its small, dont move at all
    if (speed < 0.001f) {
        velocity.x = 0;
        velocity.z = 0;
        return;
    }

    float drop = 0.0f;

    if (isGrounded) {
        glm::vec3 start = origin;
        start.x += base_velocity.x / speed * 16.0f;
        start.z += base_velocity.z / speed * 16.0f;

        glm::vec3 stop = start - glm::vec3(0, 34.0f, 0);

        BSPHullTrace edgeTrace = traceHull(start, stop, map, hullType);

        float friction = PM_FRICTION;
        if (edgeTrace.fraction == 1.0f)
            friction *= PM_EDGE_FRICTION; // about to run off a ledge — brake harder

        float control = (speed < PM_STOP_SPEED) ? PM_STOP_SPEED : speed;
        drop += control * friction * dt;
    }

    float newSpeed = std::max(speed - drop, 0.0f);
    newSpeed /= speed;

    velocity.x *= newSpeed;
    velocity.z *= newSpeed;
}

// mirrors PM_CatagorizePosition
void PlayerMovement::categorizePosition(BSPMap& map) {

    // rapidly shooting up, not on ground and can skip trace
    if (velocity.y > 180.f) {
        isGrounded = false;
        return;
    }
    
    // try and move down
    glm::vec3 point = origin - glm::vec3(0, PM_GROUNDCHECK_EPSILON, 0);
    
    BSPHullTrace trace = traceHull(origin, point, map, hullType);

    if (trace.content != BSP_CONTENTS_EMPTY && trace.normal.y >= 0.7f) {
        isGrounded = true;
        if (!trace.allSolid)
            origin = origin + (point - origin) * trace.fraction;
    } else {
        isGrounded = false;
    }
}

// mirrors PM_FlyMove — bump loop with fractional time
int PlayerMovement::flyMove(float dt, BSPMap& map) {
    
    float allFraction = 0.0f;
    glm::vec3 planes[PM_MAX_CLIP_PLANES];
    int numPlanes = 0;

    // assume we are not blocked
    int blocked = 0;

    int bumpCount;

    // total time left for movement operation
    float timeLeft = dt;

    // store original velocity
    glm::vec3 originalVelocity = velocity;
    glm::vec3 primalVelocity   = velocity;

    for (bumpCount = 0; bumpCount < PM_BUMP_COUNT; bumpCount++) {
        
        if (velocity == glm::vec3(0)) break;

        // assume we can move all the way from current origin to end point
        glm::vec3 end = origin + velocity * timeLeft;
        BSPHullTrace trace = traceHull(origin, end, map, hullType);

        // started inside solid
        if (trace.allSolid) {
            return 4;
        }

        allFraction += trace.fraction;

        if (trace.fraction > 0) {
            origin = origin + velocity * timeLeft * trace.fraction;
            originalVelocity = velocity;
            numPlanes = 0;
        }

        // if trace didnt hit anything, break;
        if (trace.content == BSP_CONTENTS_EMPTY)
            break;

        if (trace.normal.y > 0.7f) blocked |= 0x01; // floor
        if (trace.normal.y == 0.0f) blocked |= 0x02; // wall

        timeLeft *= (1.0f - trace.fraction);

        if (numPlanes >= PM_MAX_CLIP_PLANES) {
            velocity = glm::vec3(0);
            break;
        }

        planes[numPlanes++] = trace.normal;

        if (numPlanes == 1) {

            // single plane — just clip and continue
            glm::vec3 clipped;
            clipVelocity(originalVelocity, planes[0], clipped, 1.0f);
            velocity = clipped;
            originalVelocity = clipped;
        } else {
            // multi-plane handling
            int i, j;
            for (i = 0; i < numPlanes; i++) {
                glm::vec3 clipped;
                clipVelocity(originalVelocity, planes[i], clipped, 1.0f);
                velocity = clipped;
                for (j = 0; j < numPlanes; j++) {
                    if (j != i && glm::dot(velocity, planes[j]) < 0)
                        break;
                }
                if (j == numPlanes) break;
            }
            if (i == numPlanes) {
                if (numPlanes != 2) {
                    velocity = glm::vec3(0);
                    break;
                }
                glm::vec3 dir = glm::cross(planes[0], planes[1]);
                float dirLenSq = glm::dot(dir, dir);
                if (dirLenSq < 1e-6f) {
                    velocity = glm::vec3(0); // truly parallel/degenerate planes, no valid slide direction
                    break;
                }
                dir = glm::normalize(dir);
                velocity = dir * glm::dot(dir, velocity);
            }
        }

        bool skipPrimalCheck = (numPlanes == 1 && !isGrounded);

        if (!skipPrimalCheck) {
            if (glm::dot(velocity, primalVelocity) <= 0) {
                velocity = glm::vec3(0);
                break;
            }
        }
    }

    if (allFraction == 0.0f)
        velocity = glm::vec3(0);

    return blocked;
}

// mirrors PM_WalkMove
void PlayerMovement::walkMove(float dt, const glm::vec3& wishDir, BSPMap& map) {
    
    // half gravity before move (Verlet)
    velocity.y -= PM_GRAVITY * 0.5f * dt;

    // jump
    if (jumpPressed && isGrounded) {
        velocity.y = PM_JUMP_FORCE;
        velocity.y -= PM_GRAVITY * 0.5f * dt;
        isGrounded = false;
    }

    // friction on ground
    if (isGrounded) {
        velocity.y = 0.0f;
        applyFriction(dt, map);
    }

    checkVelocity();

    // get wish speed, and cap if above max speed
    float wishSpeed = glm::length(wishDir) * PM_MOVEMENT_SPEED;

    if (isDucking) {
        wishSpeed *= PM_DUCK_SPEED_MULTIPLIER;
    } else if (isSneaking) {
        wishSpeed *= PM_SNEAK_SPEED_MULTIPLIER;
    }

    glm::vec3 normalizedWish = glm::length(wishDir) > 0.001f ? glm::normalize(wishDir) : glm::vec3(0);

    // accelerate toward wish direction (uncapped if grounded, capped if in air)
    if (isGrounded)
        accelerate(normalizedWish, wishSpeed, PM_ACCELERATE, dt);
    else
        airAccelerate(normalizedWish, wishSpeed, PM_AIR_ACCELERATE, dt);

    if (isGrounded) {
        
        // grounded — do walkmove with step-up comparison
        glm::vec3 savedOrigin   = origin;
        glm::vec3 savedVelocity = velocity;

        // attempt flat move (down path)
        velocity.y = 0.0f;
        flyMove(dt, map);
        glm::vec3 downOrigin   = origin;
        glm::vec3 downVelocity = velocity;

        // reset and try step-up move (up path)
        origin   = savedOrigin;
        velocity = savedVelocity;
        velocity.y = 0.0f;

        // step up
        glm::vec3 stepUp = origin + glm::vec3(0, PM_STEPSIZE, 0);
        BSPHullTrace upTrace = traceHull(origin, stepUp, map, hullType);
        if (!upTrace.allSolid)
            origin = origin + (stepUp - origin) * upTrace.fraction;

        flyMove(dt, map);

        glm::vec3 dropDest = origin - glm::vec3(0, PM_STEPSIZE, 0);
        BSPHullTrace dropTrace = traceHull(origin, dropDest, map, hullType);

        bool useDown = false;
        if (dropTrace.normal.y < 0.7f) {
            useDown = true; // not floor
        } else if (!dropTrace.allSolid) {
            origin = origin + (dropDest - origin) * dropTrace.fraction; // snap to contact
        }

        if (useDown) {
            origin   = downOrigin;
            velocity = downVelocity;
        } else {
            glm::vec3 upOrigin = origin;
        // pick whichever went farther horizontally
        float downDist = glm::length(glm::vec2(downOrigin.x - savedOrigin.x, downOrigin.z - savedOrigin.z));
        float upDist   = glm::length(glm::vec2(upOrigin.x   - savedOrigin.x, upOrigin.z   - savedOrigin.z));

        if (downDist > upDist) {
            origin   = downOrigin;
            velocity = downVelocity;
        }
        else velocity.y = downVelocity.y;
        }

    } else {
        // airborne — just fly move
        flyMove(dt, map);
    }

    checkVelocity();

    // half gravity after move (Verlet)
    velocity.y -= PM_GRAVITY * 0.5f * dt;

    checkVelocity();

    // re-categorize ground
    categorizePosition(map);
}

bool PlayerMovement::checkStuck(BSPMap& map) {

    // performs trace for allSolid flag outcome
    const auto traceAllSolid = [&](const glm::vec3& point) -> bool {
        BSPHullTrace trace = traceHull(point, point, map, hullType);
        return trace.allSolid;
    };

    // not stuck
    if (!traceAllSolid(origin)) {
        stuckOffsetIndex = 0;
        return false;
    }

    const std::array<glm::vec3, 54> &stuckTable = createStuckTable();
    for (size_t i = 0; i < stuckTable.size(); i++) {
        int stuckTableIndex = stuckOffsetIndex % (int)stuckTable.size();
        stuckOffsetIndex++;

        glm::vec3 stuckTest = origin + stuckTable[stuckTableIndex];
        if (!traceAllSolid(stuckTest)) {
            origin = stuckTest;
            stuckOffsetIndex = 0;
            return false;
        }
    }

    // still stuck after full table
    return true;
}

void PlayerMovement::noclipMove(float dt, const glm::vec3& wishDir) {
    origin += wishDir * PM_MOVEMENT_SPEED * dt;
}

void PlayerMovement::duck(BSPMap& map) {
    if (crouchPressed) {
        isDucking = true;
    } else if (isDucking) {
        // only uncrouch if standing hull fits
        glm::vec3 standingOrigin = origin + glm::vec3(0, PM_PLAYER_ORIGIN - PM_PLAYER_ORIGIN_CROUCHED, 0);
        BSPHullTrace headSpace = traceHull(standingOrigin, standingOrigin, map, HullType::STANDING);
        if(!headSpace.allSolid) {
            origin = standingOrigin;
            isDucking = false;
        }
    }

    hullType = isDucking ? HullType::CROUCHING : HullType::STANDING;
}

void PlayerMovement::update(float dt, const glm::vec3& wishDir, BSPMap& map) {

    if (moveType != MovementType::NOCLIP) {
        duck(map);
        checkStuck(map);
    }

    switch (moveType) {
    case MovementType::WALK:
        walkMove(dt, wishDir, map);
        break;
    case MovementType::NOCLIP:
        noclipMove(dt, wishDir);
        break;
    }
}

const std::array<glm::vec3, 54>& PlayerMovement::createStuckTable() {
    static const std::array<glm::vec3, 54> stuckTable = [] {
        std::array<glm::vec3, 54> row {};
        int index = 0;

        // small single-axis nudges
        for (float v = -0.125f; v <= 0.125f; v += 0.125f) row[index++] = { 0, v, 0 }; // vertical
        for (float v = -0.125f; v <= 0.125f; v += 0.125f) row[index++] = { 0, 0, v }; // horizontal
        for (float v = -0.125f; v <= 0.125f; v += 0.125f) row[index++] = { v, 0, 0 }; // horizontal

        // small combined nudges
        for (float x = -0.125f; x <= 0.125f; x += 0.25f)
            for (float y = -0.125f; y <= 0.125f; y += 0.25f)
                for (float z = -0.125f; z <= 0.125f; z += 0.25f)
                    row[index++] = { x, y, z };

        // big single-axis nudges
        float verticalBig[3] = { 0.0f, 1.0f, 6.0f };
        for (float v : verticalBig)                          row[index++] = { 0, v, 0 };
        for (float v = -2.0f; v <= 2.0f; v += 2.0f)          row[index++] = { 0, 0, v };
        for (float v = -2.0f; v <= 2.0f; v += 2.0f)          row[index++] = { v, 0, 0 };

        // big combined nudges
        for (float y : verticalBig)
            for (float x = -2.0f; x <= 2.0f; x += 2.0f)
                for (float z = -2.0f; z <= 2.0f; z += 2.0f)
                    row[index++] = { x, y, z };

        return row;
    }();
    return stuckTable;
}

BSPHullTrace PlayerMovement::traceHull(const glm::vec3& start, const glm::vec3& end, BSPMap& map, HullType hullType) {
    return map.traceHullSegment(
        map.bsp.models()[0].iHeadnodes[(uint8_t)hullType],
        start,
        end
    );
}