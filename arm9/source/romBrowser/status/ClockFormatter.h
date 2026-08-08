#pragma once
#include "common.h"
#include "services/settings/AppSettings.h"   // ClockFormat
#include "core/mini-printf.h"                 // mini_snprintf

inline void FormatClock(char out[9], u8 hour24, u8 minute, ClockFormat fmt)
{
    if (fmt == ClockFormat::TwelveHour)
    {
        u8 h = hour24 % 12; if (h == 0) h = 12;
        const char* ap = hour24 < 12 ? "AM" : "PM";
        mini_snprintf(out, 9, "%u:%02u %s", h, minute, ap);
    }
    else
    {
        mini_snprintf(out, 9, "%02u:%02u", hour24, minute);
    }
}
