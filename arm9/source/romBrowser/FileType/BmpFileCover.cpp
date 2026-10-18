#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include <memory>
#include <libtwl/dma/dmaNitro.h>
#include "fat/File.h"
#include "core/math/ColorConverter.h"
#include "BmpHeader.h"
#include "BmpFileCover.h"

BmpFileCover::BmpFileCover(const FastFileRef& coverFileRef)
{
    const auto file = std::make_unique<File>();
    file->Open(coverFileRef, FA_READ);

    if (!file->ReadExact(_coverBuffer, 0x436) ||
        !BmpHeader::Validate(_coverBuffer, 128, 96, 8))
    {
        return;
    }

    u32 dataOffset = BmpHeader::GetDataOffset(_coverBuffer);
    if (dataOffset == 0)
    {
        return;
    }

    const bool topDown = BmpHeader::IsTopDown(_coverBuffer);

    // Only the used palette entries are stored
    u32 colorCount = BmpHeader::GetPaletteColorCount(_coverBuffer);
    memset(_palette, 0, sizeof(_palette));
    u8* paletteData32 = &_coverBuffer[0x36];
    for (u32 i = 0; i < colorCount; i++)
    {
        u32 b = *paletteData32++;
        u32 g = *paletteData32++;
        u32 r = *paletteData32++;
        paletteData32++;
        _palette[i] = ColorConverter::ToXBGR555(Rgb<5, 5, 5>(Rgb8(r, g, b)));
    }

    if (file->Seek(dataOffset) != FR_OK ||
        !file->ReadExact(_coverBuffer, sizeof(_coverBuffer)))
    {
        return;
    }

    file->Close();

    // Flip top-down bitmaps to bottom-up
    if (topDown)
    {
        u8 rowBuffer[128];
        for (u32 y = 0; y < 96 / 2; y++)
        {
            u8* topRow = &_coverBuffer[y * 128];
            u8* bottomRow = &_coverBuffer[(95 - y) * 128];
            memcpy(rowBuffer, topRow, 128);
            memcpy(topRow, bottomRow, 128);
            memcpy(bottomRow, rowBuffer, 128);
        }
    }

    DC_FlushRange(_coverBuffer, sizeof(_coverBuffer));
    DC_FlushRange(_palette, sizeof(_palette));
    _isValid = true;
}

void BmpFileCover::Upload2DCoverBitmap(void* destination) const
{
    cover_bitmapToTiledCopy(_coverBuffer, destination);
}

void BmpFileCover::Upload2DCoverPalette(void* destination) const
{
    dma_ntrCopy32(3, _palette, destination, sizeof(_palette));
}
