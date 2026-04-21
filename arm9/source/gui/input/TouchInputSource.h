#pragma once
#include "common.h"
#include <nds/input.h>
#include "ITouchInputSource.h"
#include "sharedMemory.h"

/// @brief Input source from the physical DS touch screen.
class TouchInputSource : public ITouchInputSource
{
public:
    bool Sample(Point& touchPosition) const override
    {
        if (SHARED_KEY_XY & (1 << 6))
        {
            touchPosition = Point(0, 0);
            return false;
        }
        else
        {
            touchPosition = Point(SHARED_TOUCH_X, SHARED_TOUCH_Y);
            return true;
        }
    }
};
