#pragma once
#include "common.h"
#include <nds/input.h>
#include "IInputSource.h"
#include "sharedMemory.h"

/// @brief Input source from the physical DS buttons.
class PadInputSource : public IInputSource
{
public:
    InputKey Sample() const override
    {
        if (SHARED_SYSTEM_FLAGS & SHARED_FLAG_SLEEP_MODE)
            return InputKey::None;

        u16 arm9Mask = (~REG_KEYINPUT & 0x3FF);
        u16 arm7Mask = (~SHARED_KEY_XY & 0xB);
        return static_cast<InputKey>((arm9Mask | (arm7Mask << 10)));
    }
};
