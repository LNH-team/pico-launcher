#pragma once
#include "core/math/Point.h"

/// @brief Type of touch event.
enum class TouchEventType
{
    /// @brief Pen first touched the screen.
    Down,
    /// @brief Pen moved while touching.
    Move,
    /// @brief Pen was lifted.
    Up
};

/// @brief Describes a touch screen event.
struct TouchEvent
{
    /// @brief The type of event.
    TouchEventType type;
    /// @brief The current touch position on screen.
    Point position;
    /// @brief The position where the touch started (for Down, same as position).
    Point startPosition;
    /// @brief Movement since the last frame.
    int deltaX;
    int deltaY;
    /// @brief Velocity in pixels per frame (fixed point 4.4).
    int velocityX;
    int velocityY;
    /// @brief Total movement since touch start.
    int totalDeltaX;
    int totalDeltaY;
    /// @brief Number of frames this touch has been held.
    int holdFrames;
};
