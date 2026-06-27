#pragma once
#include <cstdlib>
#include <nds/ndstypes.h>

// Validates BITMAPFILEHEADER + BITMAPINFOHEADER fields from a raw BMP buffer.
// buf must be at least 0x32 bytes (50 bytes covers all validated fields).
// Accepts both bottom-up (positive height) and top-down (negative height) BMPs.
struct BmpHeader
{
    static s32 GetHeight(const u8* buf)
    {
        return (s32)(buf[0x16] | (buf[0x17] << 8) | (buf[0x18] << 16) | (buf[0x19] << 24));
    }

    static bool Validate(const u8* buf, u32 expectedWidth, u32 expectedHeight, u32 expectedBpp)
    {
        if (buf[0] != 'B' || buf[1] != 'M')
            return false;

        u32 dibSize = buf[0x0E] | (buf[0x0F] << 8) | (buf[0x10] << 16) | (buf[0x11] << 24);
        u32 width   = buf[0x12] | (buf[0x13] << 8) | (buf[0x14] << 16) | (buf[0x15] << 24);
        u32 bpp     = buf[0x1C] | (buf[0x1D] << 8);
        u32 comp    = buf[0x1E] | (buf[0x1F] << 8) | (buf[0x20] << 16) | (buf[0x21] << 24);
        u32 clrUsed = buf[0x2E] | (buf[0x2F] << 8) | (buf[0x30] << 16) | (buf[0x31] << 24);

        return dibSize == 40
            && width == expectedWidth
            && (u32)std::abs(GetHeight(buf)) == expectedHeight
            && bpp == expectedBpp
            && comp == 0
            && (clrUsed == 0 || clrUsed == (1u << expectedBpp));
    }

    // Returns true if the BMP stores rows top-to-bottom (negative biHeight).
    // Call only after Validate() succeeds.
    static bool IsTopDown(const u8* buf)
    {
        return GetHeight(buf) < 0;
    }
};
