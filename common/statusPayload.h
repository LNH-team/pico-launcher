#pragma once
#include "common.h"
/// @brief Resolves on both ARM7 and ARM9 via the same relative include pattern used by rtcIpc.h.
#include <../../libtwl7/include/libtwl/sio/sioRtc.h>

/// @brief Shared ARM7 -> ARM9 status payload.
struct StatusPayload
{
    rtc_datetime_t dateTime;  ///< Hour/minute decoded to decimal by ARM7; date fields remain BCD.
    u8 batteryLevel;          ///< DSi: 0-15 (raw & 0x0F); DS: 0=ok, 1=low.
    u8 flags;                 ///< Bit 0 = charging, bit 1 = has fine level.
};
/// @brief Battery is charging.
#define STATUS_FLAG_CHARGING       (1 << 0)
/// @brief Battery level comes from the DSi fine-grained register.
#define STATUS_FLAG_HAS_FINE_LEVEL (1 << 1)
