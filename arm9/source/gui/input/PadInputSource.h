#pragma once
#include "common.h"
#include <nds/input.h>
#include "IKeyInputSource.h"
#include "sharedMemory.h"

/// @brief Input source from the physical DS buttons.
class PadInputSource : public IKeyInputSource
{
public:
    InputKey Sample() const override
    {
        u16 arm9Mask = (~REG_KEYINPUT & 0x3FF);
        u16 arm7Mask = (~SHARED_KEY_XY & 0xCB);
        arm7Mask = (arm7Mask & 0xB) | ((arm7Mask & 0xC0) >> 2);
        return static_cast<InputKey>((arm9Mask | (arm7Mask << 10)));
    }
};
