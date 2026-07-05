#pragma once
#include <cstdlib>
#include <nds/ndstypes.h>

/// @brief Static helpers for reading and validating BITMAPFILEHEADER + BITMAPINFOHEADER
///        fields from a raw BMP buffer.
struct BmpHeader
{
    /// @brief Reads the (possibly negative) biHeight field of a BMP's DIB header.
    /// @param bmpHeader Buffer containing BMP data. Must be at least 50 bytes.
    /// @return The image height. Negative when the BMP is stored top-down.
    static s32 GetHeight(const u8* bmpHeader)
    {
        return (s32)(bmpHeader[0x16] | (bmpHeader[0x17] << 8) | (bmpHeader[0x18] << 16) | (bmpHeader[0x19] << 24));
    }

    /// @brief Validates BITMAPFILEHEADER + BITMAPINFOHEADER fields from a raw BMP buffer.
    /// @param bmpHeader Buffer containing BMP data. Must be at least 50 bytes.
    /// @param expectedWidth Expected width of the image.
    /// @param expectedHeight Expected height of the image.
    /// @param expectedBpp Expected bits per pixel.
    /// @return \c true when valid, or \c false otherwise.
    static bool Validate(const u8* bmpHeader, u32 expectedWidth, u32 expectedHeight, u32 expectedBpp)
    {
        if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M')
        {
            return false;
        }

        u32 dibSize = bmpHeader[0x0E] | (bmpHeader[0x0F] << 8) | (bmpHeader[0x10] << 16) | (bmpHeader[0x11] << 24);
        u32 width   = bmpHeader[0x12] | (bmpHeader[0x13] << 8) | (bmpHeader[0x14] << 16) | (bmpHeader[0x15] << 24);
        u32 bpp     = bmpHeader[0x1C] | (bmpHeader[0x1D] << 8);
        u32 comp    = bmpHeader[0x1E] | (bmpHeader[0x1F] << 8) | (bmpHeader[0x20] << 16) | (bmpHeader[0x21] << 24);
        u32 clrUsed = bmpHeader[0x2E] | (bmpHeader[0x2F] << 8) | (bmpHeader[0x30] << 16) | (bmpHeader[0x31] << 24);

        return dibSize == 40
            && width == expectedWidth
            && (u32)std::abs(GetHeight(bmpHeader)) == expectedHeight
            && bpp == expectedBpp
            && comp == 0
            && (clrUsed == 0 || clrUsed == (1u << expectedBpp));
    }

    /// @brief Returns \c true if the BMP stores rows top-to-bottom (negative biHeight).
    /// @param bmpHeader The raw BMP buffer.
    /// @return \c true when top-down, or \c false otherwise.
    /// @note Call only after Validate() succeeds.
    static bool IsTopDown(const u8* bmpHeader)
    {
        return GetHeight(bmpHeader) < 0;
    }
};
