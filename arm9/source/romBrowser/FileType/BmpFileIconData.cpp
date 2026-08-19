#include "common.h"
#include <memory>
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "core/math/ColorConverter.h"
#include "BmpHeader.h"
#include "BmpFileIconData.h"

BmpFileIconData::BmpFileIconData(const FastFileRef& iconFileRef)
{
    auto file = std::make_unique<File>();
    file->Open(iconFileRef, FA_READ);

    memset(_iconGfx, 0, sizeof(_iconGfx));
    memset(_iconPltt, 0, sizeof(_iconPltt));
    memset(_iconCoverPltt, 0, sizeof(_iconCoverPltt));
    _isValid = Load(std::move(file));
    DC_FlushRange(_iconGfx, sizeof(_iconGfx));
    DC_FlushRange(_iconPltt, sizeof(_iconPltt));
    DC_FlushRange(_iconCoverPltt, sizeof(_iconCoverPltt));
}

bool BmpFileIconData::Load(std::unique_ptr<File> file)
{
    // BMP file header (14) + DIB header (40) + 16-color palette (64)
    u8 headerAndPalette[118];
    if (!file->ReadExact(headerAndPalette, sizeof(headerAndPalette)) ||
        !BmpHeader::Validate(headerAndPalette, 32, 32, 4))
    {
        return false;
    }

    u32 dataOffset = headerAndPalette[0xA] | (headerAndPalette[0xB] << 8) |
        (headerAndPalette[0xC] << 16) | (headerAndPalette[0xD] << 24);

    if (dataOffset < sizeof(headerAndPalette))
    {
        return false;
    }

    const bool topDown = BmpHeader::IsTopDown(headerAndPalette);

    const u8* paletteData = &headerAndPalette[0x36];
    for (u32 i = 0; i < 16; i++)
    {
        u32 b = *paletteData++;
        u32 g = *paletteData++;
        u32 r = *paletteData++;
        paletteData++;
        Rgb<8, 8, 8> color(r, g, b);
        _iconPltt[i] = ColorConverter::ToGBGR565(color);
        _iconCoverPltt[i] = ColorConverter::ToXBGR555(Rgb<5, 5, 5>(color));
    }

    // Heap-allocate the staging buffer so it doesn't live on the task thread stack.
    auto rawPixelData = std::make_unique<u8[]>(GfxSize);
    if (!rawPixelData ||
        file->Seek(dataOffset) != FR_OK ||
        !file->ReadExact(rawPixelData.get(), GfxSize))
    {
        memset(_iconPltt, 0, sizeof(_iconPltt));
        memset(_iconCoverPltt, 0, sizeof(_iconCoverPltt));
        return false;
    }

    // Convert BMP rows (bottom-up or top-down) to the DS tiled 4 bpp sprite format.
    // BMP is high-nibble-first; DS tiles are low-nibble-first -- swap nibbles per 4-byte group.
    for (int y = 0; y < 32; y++)
    {
        // Bottom-up BMP (normal, positive height): row 0 is the bottom of the image.
        // Top-down BMP (negative height): row 0 is the top of the image.
        const u8* srcRowPtr = topDown
            ? rawPixelData.get() + y * 16
            : rawPixelData.get() + (31 - y) * 16;

        int ty = y / 8;
        int py = y % 8;

        for (int tx = 0; tx < 4; tx++)
        {
            u32 val;
            memcpy(&val, srcRowPtr + tx * 4, 4);
            val = ((val >> 4) & 0x0F0F0F0F) | ((val & 0x0F0F0F0F) << 4);
            memcpy(&_iconGfx[(ty * 4 + tx) * 32 + py * 4], &val, 4);
        }
    }

    return true;
}
