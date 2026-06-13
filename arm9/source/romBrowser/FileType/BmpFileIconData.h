#pragma once
#include "fat/FastFileRef.h"

class File;

/// @brief Decoded graphics and palette of a 32x32 4 bpp 16-color BMP file icon,
///        loaded once and shared by BmpFileIcon instances.
class alignas(32) BmpFileIconData
{
public:
    static constexpr u32 GfxSize = 512;

    explicit BmpFileIconData(const FastFileRef& iconFileRef);
    explicit BmpFileIconData(const TCHAR* path);

    const u8* GetGfx() const { return _iconGfx; }
    const u16* GetPltt() const { return _iconPltt; }

private:
    void Init(File& file);
    void Load(File& file);

    u8 _iconGfx[GfxSize] alignas(32);
    u16 _iconPltt[16] alignas(32);
};
