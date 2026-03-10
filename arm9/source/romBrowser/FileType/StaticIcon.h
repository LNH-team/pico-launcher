#pragma once
#include "FileIcon.h"

class StaticIcon : public FileIcon
{
public:
    StaticIcon(const u8* tileData, const u16* paletteData)
        : _tileData(tileData), _paletteData(paletteData) { }

    void UploadGraphics(vu16* vram) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor) override;

private:
    const u8* _tileData;
    const u16* _paletteData;
    u32 _vramOffset;
};