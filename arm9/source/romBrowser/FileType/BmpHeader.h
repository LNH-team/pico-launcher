#pragma once
#include <nds/ndstypes.h>

// Validates BITMAPFILEHEADER + BITMAPINFOHEADER fields from a raw BMP buffer.
// buf must be at least 0x22 bytes (34 bytes covers all validated fields).
struct BmpHeader
{
    static bool Validate(const u8* buf, u32 expectedWidth, u32 expectedHeight, u32 expectedBpp)
    {
        if (buf[0] != 'B' || buf[1] != 'M')
            return false;

        u32 width  = buf[0x12] | (buf[0x13] << 8) | (buf[0x14] << 16) | (buf[0x15] << 24);
        u32 height = buf[0x16] | (buf[0x17] << 8) | (buf[0x18] << 16) | (buf[0x19] << 24);
        u32 bpp    = buf[0x1C] | (buf[0x1D] << 8);
        u32 comp   = buf[0x1E] | (buf[0x1F] << 8) | (buf[0x20] << 16) | (buf[0x21] << 24);

        return width == expectedWidth && height == expectedHeight && bpp == expectedBpp && comp == 0;
    }
};
