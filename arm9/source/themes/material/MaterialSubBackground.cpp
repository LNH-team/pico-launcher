#include "common.h"
#include <nds/arm9/background.h>
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/mem/memVram.h>
#include "core/math/RgbMixer.h"
#include "mainBg.h"
#include "MaterialSubBackground.h"

void MaterialSubBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    mem_setVramHMapping(MEM_VRAM_H_LCDC);
    RgbMixer::MakeGradientPalette((u16*)0x06898020,
        _materialColorScheme->inverseOnSurface, _materialColorScheme->secondaryContainer);
    mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
    dma_ntrCopy32(3, mainBgTiles, (vu8*)BG_GFX_SUB + 0x8000, mainBgTilesLen);
    dma_ntrCopy32(3, mainBgMap, (vu8*)BG_GFX_SUB + 0x8800, mainBgMapLen);
}

void MaterialSubBackground::VBlank()
{
    REG_DISPCNT_SUB = (REG_DISPCNT_SUB & ~((0xF) | (1 << 10))) | 5 | (1 << 8);
    REG_BG0CNT_SUB = BG_32x32 | BG_PRIORITY_3 | BG_COLOR_256 | BG_MAP_BASE(17) | BG_TILE_BASE(2);
    REG_BG0HOFS_SUB = 0;
    REG_BG0VOFS_SUB = 0;
}
