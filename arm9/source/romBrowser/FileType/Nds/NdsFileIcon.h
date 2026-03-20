#pragma once
#include "../FileIcon.h"
#include "ndsBanner.h"

/// @brief Icon of a nds rom.
class NdsFileIcon : public FileIcon
{
public:
    explicit NdsFileIcon(const nds_banner_t* banner);

    void UploadGraphics(vu16* vram) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor) override;

private:
    const nds_banner_t* _banner;
    bool _animated;
    int _animTokenIdx;
    // int _durationCounter;
    u32 _lastAnimToken;
    u32 _animLength;
    u16 _tokenStartTimes[65];
    bool _loop;

    vu16* _vramAddress = nullptr;
    int _currentVramSlot = 0;
    int _currentGfxIdx = -1;
};