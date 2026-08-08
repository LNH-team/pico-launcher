#pragma once
#include "statusPayload.h"

enum class BatteryIcon { Empty, Low, Medium, High, Full, Charging };

/// @brief Whether a battery icon should render in the warning (red) ink.
/// @details GBATEK's own wording for the DSi's low-battery tiers is "one solid red bar" (0x3..0x6)
/// and "one blinking red bar" (0x1..0x2) -- both explicitly red, confirmed against the physical
/// power LED on real hardware (also red at the solid-bar tier). Empty (0x0..0x2, the practical
/// floor -- see ResolveBatteryIcon) is a warning too; nothing else is.
inline bool IsWarning(BatteryIcon icon)
{
    return icon == BatteryIcon::Low || icon == BatteryIcon::Empty;
}

inline BatteryIcon ResolveBatteryIcon(const StatusPayload& s)
{
    if (s.flags & STATUS_FLAG_CHARGING) return BatteryIcon::Charging; // bolt overrides, even at full

    if (s.flags & STATUS_FLAG_HAS_FINE_LEVEL)
    {
        // The DSi reports icon step markers in bits 0-3 of BPTWL reg 0x20: 0xF=full, 0xB=3 bars,
        // 0x7=2 bars, 0x3=1 solid red bar, 0x1=1 blinking red bar, 0x0=critical/shutdown.
        // Match the native menu by reserving the Full icon for 0xF itself rather than rounding
        // 0xD/0xE up to Full. The level is only meaningful when not charging - the charging branch
        // above already returns before we get here.
        //
        // 0x1 and 0x0 are GBATEK-documented but not treated as distinct tiers here: two real-hardware
        // tests (once at ~1 poll/s, once at ~10 polls/s) never observed either value before the
        // console lost power, so whatever cuts it off (likely the battery pack's own protection
        // circuit) happens at or above 0x2. 0x2 and below are treated as one practical floor tier.
        u8 level = s.batteryLevel;
        if (level >= 0x0F) return BatteryIcon::Full;     // 0xF
        if (level >= 0x0B) return BatteryIcon::High;     // 0xB..0xE
        if (level >= 0x07) return BatteryIcon::Medium;   // 0x7..0xA
        if (level >= 0x03) return BatteryIcon::Low;      // 0x3..0x6: solid red bar
        return BatteryIcon::Empty;                        // 0x0..0x2: practical floor
    }
    // DS / DS-Lite binary: 1 = low.
    return s.batteryLevel ? BatteryIcon::Low : BatteryIcon::Full;
}
