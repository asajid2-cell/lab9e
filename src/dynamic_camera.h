#ifndef DYNAMIC_CAMERA_H
#define DYNAMIC_CAMERA_H

#include <iostream>
#include <limits>

#include "camera.h"

constexpr float INF = std::numeric_limits<float>::infinity();

// AABB in 1D
struct Interval {
    float min;
    float max;

    constexpr bool contains(const Interval& other) const {
        return (min <= other.min) && (max >= other.max);
    }

    constexpr bool contains(float x) const { return (min <= x) && (x <= max); }

    // signedDistance returns a positive smallest distance of x from the interval boundary
    // if it is not contained, else it returns a smallest (in magnitude) negative distance
    // from the interval boundary.
    constexpr float signedDistance(float x) const {
        if (contains(x)) {
            // 0 or negative
            return std::max(x - max, min - x);
        }
        // positive
        return std::max(x - max, min - x);
    }
};

struct DynamicCamera {
    void update(Camera& camera,                           // the camera to update
                const sf::Vector2f& trackedPosWorld,      // world coords of player to track
                const sf::Vector2f& trackedVelocityWorld  // world velocity of player to track
    ) {
        // Where the player is on screen, as a percentage of the window width, if the camera sat
        // at its unadjusted center: the current camera is shifted by the lookahead, so add that
        // shift back. The zones are measured against the unadjusted center so that lookahead
        // never feeds back into them.
        const float playerXPercent =
            camera.worldToWindowPercent(trackedPosWorld).x +
            camera.worldToWindowPercentDist({lookaheadState, 0.f}).x;

        // Lookahead eases toward lookAhead of the window width in the direction of travel, and
        // back to zero when the player stops.
        float direction = 0.f;
        if (trackedVelocityWorld.x > 0.f) {
            direction = 1.f;
        } else if (trackedVelocityWorld.x < 0.f) {
            direction = -1.f;
        }
        const float lookaheadTarget =
            direction * camera.windowPercentToWorldDist({lookAhead, 0.f}).x;
        lookaheadState += alpha * (lookaheadTarget - lookaheadState);

        // Dead zone: no move. Soft zone: ease a fraction of the way to the nearest dead zone edge.
        // Hard zone: move the whole way to the nearest soft zone edge. Moving the camera by d
        // percent moves the player by -d, so the correction is (player - edge).
        float deltaXPercent = 0.f;
        if (!deadZoneX.contains(playerXPercent)) {
            const bool soft = softZoneX.contains(playerXPercent);
            const Interval& edge = soft ? deadZoneX : softZoneX;
            const float boundary = (playerXPercent < edge.min) ? edge.min : edge.max;
            deltaXPercent = (soft ? alpha : 1.f) * (playerXPercent - boundary);
        }
        cameraUnadjustedCenter.x += camera.windowPercentToWorldDist({deltaXPercent, 0.f}).x;

        // Draw from the unadjusted center plus the smoothed lookahead.
        camera.setCameraCenter(
            {cameraUnadjustedCenter.x + lookaheadState, cameraUnadjustedCenter.y});
    }

    float alpha;  // alpha to use for damping of camera position, *as well as* for smoothing of
                  // lookahead offset.
    Interval deadZoneX;  // percentage interval along x-axis to use for dead zone in [0, 1]
    // percentage interval along x-axis to use for soft rejection zone in [0, 1]
    Interval softZoneX = {-INF, INF};
    // (everything beyond softZone is considered hard zone).
    float lookAhead;  // screen-percentage lookahead
    mutable sf::Vector2f
        cameraUnadjustedCenter;      // the camera world center from last frame, ignoring lookahead
    mutable float lookaheadState{};  // the EMA of the accumulated lookahead offset in world coords
};

#endif // DYNAMIC_CAMERA_H
