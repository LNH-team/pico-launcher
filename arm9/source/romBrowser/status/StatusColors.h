#pragma once
#include "core/math/Rgb.h"

// Ink color used to flag a low/empty battery. MD3 error tones: #B3261E on a light
// scheme, #FFB4AB on a dark one - picked so the warning reads on the strip background.
inline Rgb<8, 8, 8> StatusWarningColor(bool darkTheme)
{
    return darkTheme
        ? Rgb<8, 8, 8>(0xFF, 0xB4, 0xAB)  // MD3 error80 (#FFB4AB)
        : Rgb<8, 8, 8>(0xB3, 0x26, 0x1E); // MD3 error40 (#B3261E)
}
