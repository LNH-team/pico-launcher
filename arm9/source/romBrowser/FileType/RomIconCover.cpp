#include "common.h"
#include <string.h>
#include <memory>
#include <nds/arm9/cache.h>
#include <libtwl/dma/dmaNitro.h>
#include "RomIconCover.h"

RomIconCover::RomIconCover(const u8* iconGfx, const u16* iconPalette, u16 backgroundColor, bool large)
{
    memset(_coverBuffer, 0, sizeof(_coverBuffer));
    memset(_palette, 0, sizeof(_palette));

    // Copy the 16-color palette to the first 16 slots of our 256-color palette
    if (iconPalette)
    {
        memcpy(_palette, iconPalette, 16 * sizeof(u16));
    }

    // Slot 0 is the cover background around the icon (the icon's own color 0
    // is never referenced, its pixels are remapped to slot 16 below). Slot 16
    // is white and stands in for the icon's transparent color 0.
    _palette[0] = backgroundColor;
    _palette[16] = 0x7FFF;

    // Decode 32x32 4bpp DS tiled format to a temporary 32x32 8bpp linear buffer
    auto tempIcon = std::make_unique<u8[]>(32 * 32);
    if (iconGfx)
    {
        for (int ty = 0; ty < 4; ty++)
        {
            for (int tx = 0; tx < 4; tx++)
            {
                const u8* tileGfx = iconGfx + (ty * 4 + tx) * 32;

                for (int py = 0; py < 8; py++)
                {
                    int targetY = ty * 8 + py;
                    u8* targetRow = tempIcon.get() + targetY * 32 + tx * 8;
                    const u8* tileRowBytes = tileGfx + py * 4;

                    for (int px = 0; px < 4; px++)
                    {
                        u8 b = tileRowBytes[px];
                        u8 p1 = b & 0x0F;
                        u8 p2 = (b >> 4) & 0x0F;
                        targetRow[px * 2] = (p1 == 0) ? 16 : p1;
                        targetRow[px * 2 + 1] = (p2 == 0) ? 16 : p2;
                    }
                }
            }
        }
    }

    if (large)
    {
        // 3x integer scaling (96x96), centered horizontally, to keep pixel art scaling even
        for (int screenY = 0; screenY < 96; screenY++)
        {
            int targetY = 95 - screenY; // bottom-up layout
            int srcY = screenY / 3;
            u8* targetRow = _coverBuffer + targetY * 128 + 5;
            const u8* srcRow = tempIcon.get() + srcY * 32;

            for (int screenX = 0; screenX < 96; screenX++)
            {
                targetRow[screenX] = srcRow[screenX / 3];
            }
        }
    }
    else
    {
        // Draw the 32x32 icon centered
        for (int y = 0; y < 32; y++)
        {
            int targetY = 95 - (32 + y); // bottom-up layout
            memcpy(_coverBuffer + targetY * 128 + 37, tempIcon.get() + y * 32, 32);
        }
    }

    DC_FlushRange(_coverBuffer, sizeof(_coverBuffer));
    DC_FlushRange(_palette, sizeof(_palette));
}

void RomIconCover::Upload2DCoverBitmap(void* destination) const
{
    cover_bitmapToTiledCopy(_coverBuffer, destination);
}

void RomIconCover::Upload2DCoverPalette(void* destination) const
{
    dma_ntrCopy32(3, _palette, destination, sizeof(_palette));
}
