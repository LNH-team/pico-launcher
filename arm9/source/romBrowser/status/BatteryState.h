#pragma once
#include "statusPayload.h"

/// @brief Battery icon displayed by the Material status view.
enum class BatteryIcon { Empty, Low, Medium, High, Full, Charging };

/// @brief Battery icon and its warning presentation.
struct BatteryDisplay
{
    BatteryIcon icon;
    bool warning;
    bool blink;
};

/// @brief Maps the raw hardware status to a display icon and warning state.
inline BatteryDisplay ResolveBatteryDisplay(const StatusPayload& status)
{
    if (status.flags & STATUS_FLAG_CHARGING)
    {
        return { BatteryIcon::Charging, false, false };
    }

    if (status.flags & STATUS_FLAG_HAS_FINE_LEVEL)
    {
        u8 level = status.batteryLevel;
        if (level >= 0x0F)
        {
            return { BatteryIcon::Full, false, false };
        }
        if (level >= 0x0B)
        {
            return { BatteryIcon::High, false, false };
        }
        if (level >= 0x07)
        {
            return { BatteryIcon::Medium, false, false };
        }
        if (level >= 0x03)
        {
            return { BatteryIcon::Low, true, false };
        }
        if (level >= 0x01)
        {
            return { BatteryIcon::Low, true, true };
        }
        return { BatteryIcon::Empty, true, true };
    }

    return status.batteryLevel
        ? BatteryDisplay { BatteryIcon::Low, true, false }
        : BatteryDisplay { BatteryIcon::Full, false, false };
}
