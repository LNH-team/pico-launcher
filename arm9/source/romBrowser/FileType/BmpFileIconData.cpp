#include "common.h"
#include <memory>
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "core/math/ColorConverter.h"
#include "BmpFileIconData.h"
#include "BmpHeader.h"

BmpFileIconData::BmpFileIconData(const FastFileRef& iconFileRef)
{
    File file;
    file.Open(iconFileRef, FA_READ);
    Init(file);
}

BmpFileIconData::BmpFileIconData(const TCHAR* path)
{
    File file;
    file.Open(path, FA_READ);
    Init(file);
}

void BmpFileIconData::Init(File& file)
{
    memset(_iconGfx, 0, sizeof(_iconGfx));
    memset(_iconPltt, 0, sizeof(_iconPltt));
    Load(file);
    DC_FlushRange(_iconGfx, sizeof(_iconGfx));
    DC_FlushRange(_iconPltt, sizeof(_iconPltt));
}

void BmpFileIconData::Load(File& file)
{
    // BMP file header (14) + DIB header (40) + 16-color palette (64)
    u8 headerAndPalette[118];
    if (!file.ReadExact(headerAndPalette, sizeof(headerAndPalette)))
        return;

    if (!BmpHeader::Validate(headerAndPalette, 32, 32, 4))
        return;

    u32 dataOffset = headerAndPalette[0xA] | (headerAndPalette[0xB] << 8) |
        (headerAndPalette[0xC] << 16) | (headerAndPalette[0xD] << 24);

    if (dataOffset < sizeof(headerAndPalette))
        return;

    const bool topDown = BmpHeader::IsTopDown(headerAndPalette);

    const u8* paletteData = &headerAndPalette[0x36];
    for (u32 i = 0; i < 16; i++)
    {
        u32 b = *paletteData++;
        u32 g = *paletteData++;
        u32 r = *paletteData++;
        paletteData++;
        _iconPltt[i] = ColorConverter::ToXBGR555(Rgb<5, 5, 5>(Rgb<8, 8, 8>(r, g, b)));
    }

    // Heap-allocate the staging buffer so it doesn't live on the task thread stack.
    auto rawPixelData = std::make_unique<u8[]>(GfxSize);
    if (!rawPixelData)
    {
        memset(_iconPltt, 0, sizeof(_iconPltt));
        return;
    }
    if (file.Seek(dataOffset) != FR_OK ||
        !file.ReadExact(rawPixelData.get(), GfxSize))
    {
        memset(_iconPltt, 0, sizeof(_iconPltt));
        return;
    }

    // Convert BMP rows (bottom-up or top-down) to the DS tiled 4 bpp sprite format.
    for (int y = 0; y < 32; y++)
    {
        // Bottom-up BMP (normal, positive height): row 0 is the bottom of the image.
        // Top-down BMP (negative height): row 0 is the top of the image.
        const u8* srcRowPtr = topDown
            ? rawPixelData.get() + y * 16
            : rawPixelData.get() + (31 - y) * 16;

        int ty = y / 8;
        int py = y % 8;

        for (int x = 0; x < 32; x++)
        {
            int tx = x / 8;
            int px = x % 8;

            // BMP stores the high nibble first
            u8 byteVal = srcRowPtr[x / 2];
            u8 colorIndex = (x % 2 == 0) ? (byteVal >> 4) : (byteVal & 0x0F);

            // The DS sprite tile format stores the low nibble first
            int tileIdx = ty * 4 + tx;
            int destByteOffset = tileIdx * 32 + py * 4 + px / 2;
            if (px % 2 == 0)
                _iconGfx[destByteOffset] = (_iconGfx[destByteOffset] & 0xF0) | colorIndex;
            else
                _iconGfx[destByteOffset] = (_iconGfx[destByteOffset] & 0x0F) | (colorIndex << 4);
        }
    }
}
