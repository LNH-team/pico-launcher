#pragma once
#include "FileCover.h"

/// @brief Class representing a ROM/folder icon fallback cover.
class alignas(32) RomIconCover : public FileCover
{
public:
    RomIconCover(const u8* iconGfx, const u16* iconPalette, u16 backgroundColor, bool large);

    VBlankTextureLoadRequest CreateTextureLoadRequest() const override
    {
        return VBlankTextureLoadRequest(
            _coverBuffer, 128 * 96, _texVramOffset,
            _palette, sizeof(_palette), _plttVramOffset,
            nullptr, nullptr);
    }

    void Upload2DCoverBitmap(void* destination) const override;
    void Upload2DCoverPalette(void* destination) const override;

    bool IsActualCover() const override { return false; }

private:
    u8 _coverBuffer[128 * 96] alignas(32);
    u16 _palette[256] alignas(32);
};
