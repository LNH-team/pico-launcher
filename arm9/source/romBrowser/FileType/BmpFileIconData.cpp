#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "core/math/ColorConverter.h"
#include "BmpFileIconData.h"
#include "BmpHeader.h"

BmpFileIconData::BmpFileIconData(const FastFileRef& iconFileRef)
{
    memset(_iconGfx, 0, sizeof(_iconGfx));
    memset(_iconPltt, 0, sizeof(_iconPltt));
    File file;
    file.Open(iconFileRef, FA_READ);
    Load(file);
    DC_FlushRange(_iconGfx, sizeof(_iconGfx));
    DC_FlushRange(_iconPltt, sizeof(_iconPltt));
}

BmpFileIconData::BmpFileIconData(const TCHAR* path)
{
    memset(_iconGfx, 0, sizeof(_iconGfx));
    memset(_iconPltt, 0, sizeof(_iconPltt));
    File file;
    file.Open(path, FA_READ);
    Load(file);
    DC_FlushRange(_iconGfx, sizeof(_iconGfx));
    DC_FlushRange(_iconPltt, sizeof(_iconPltt));
}

void BmpFileIconData::Load(File& file)
{
    // BMP header + 16-color palette (14 + 40 + 64 bytes)
    u8 headerAndPalette[118];
    if (!file.ReadExact(headerAndPalette, sizeof(headerAndPalette)))
        return;

    if (!BmpHeader::Validate(headerAndPalette, 32, 32, 4))
        return;

    u32 dataOffset = headerAndPalette[0xA] | (headerAndPalette[0xB] << 8) |
        (headerAndPalette[0xC] << 16) | (headerAndPalette[0xD] << 24);

    if (dataOffset < sizeof(headerAndPalette))
        return;

    const u8* paletteData = &headerAndPalette[0x36];
    for (u32 i = 0; i < 16; i++)
    {
        u32 b = *paletteData++;
        u32 g = *paletteData++;
        u32 r = *paletteData++;
        paletteData++;
        _iconPltt[i] = ColorConverter::ToXBGR555(Rgb<5, 5, 5>(Rgb<8, 8, 8>(r, g, b)));
    }

    u8 rawPixelData[512];
    if (file.Seek(dataOffset) != FR_OK ||
        !file.ReadExact(rawPixelData, sizeof(rawPixelData)))
    {
        memset(_iconPltt, 0, sizeof(_iconPltt));
        return;
    }

    // Convert linear bottom-to-top rows to the DS tiled 4 bpp sprite format
    for (int y = 0; y < 32; y++)
    {
        const u8* srcRowPtr = rawPixelData + (31 - y) * 16; // 32 pixels @ 4 bpp = 16 bytes/row
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
            {
                _iconGfx[destByteOffset] = (_iconGfx[destByteOffset] & 0xF0) | colorIndex;
            }
            else
            {
                _iconGfx[destByteOffset] = (_iconGfx[destByteOffset] & 0x0F) | (colorIndex << 4);
            }
        }
    }
}
