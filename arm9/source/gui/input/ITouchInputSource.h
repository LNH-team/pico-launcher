#pragma once
#include "core/math/Point.h"

/// @brief Interface for a source of touch input.
class ITouchInputSource
{
public:
    virtual ~ITouchInputSource() { }

    /// @brief Samples the touch input.
    /// @param touchPosition When the pen is down, the current touch position.
    /// @return \c true when the pen is down, or \c false otherwise.
    virtual bool Sample(Point& touchPosition) const = 0;
};
