#pragma once
#include <memory>
#include "animation/Animator.h"
#include "core/math/Rgb.h"
#include "gui/views/View.h"
#include "themes/IFontRepository.h"
#include "themes/material/MaterialColorScheme.h"

class GraphicsContext;
class VramContext;

class BgmNowPlayingView : public View
{
public:
    BgmNowPlayingView(const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    void SetBgmName(const char* name);

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, kMaxBaseWidth, kBaseHeight);
    }

private:
    enum class State
    {
        Hidden,
        FadeIn,
        Hold,
        FadeOut
    };

    static constexpr int kScreenWidth = 256;
    static constexpr int kBaseHeight = 20;
    static constexpr int kPaddingX = 10;
    static constexpr int kPaddingY = 3;
    static constexpr int kVerticalOffset = 4; 
    static constexpr int kMinBaseWidth = 64;
    static constexpr int kMaxBaseWidth = 220;
    static constexpr int kMaxNameLength = 64;
    static constexpr int kBannerR = 255;
    static constexpr int kBannerG = 255;
    static constexpr int kBannerB = 255;
    static constexpr int kTextR = 0;
    static constexpr int kTextG = 0;
    static constexpr int kTextB = 0;
    static constexpr int kHoldFrames = 180;
    static constexpr int kLabelHeight = 16;
    static constexpr int kLabelMaxWidth = kMaxBaseWidth - (kPaddingX * 2);

    void StartFadeOut();
    void UpdateBaseWidth();
    void UpdateLabelTileBuffer();

    const MaterialColorScheme* _materialColorScheme;
    const nft2_header_t* _font;

    u32 _chipVramOffset = 0;

    int _baseWidth = kMinBaseWidth;
    u32 _labelRenderWidth = kLabelMaxWidth;
    u32 _labelActualWidth = 0;
    u32 _labelActualHeight = 0;
    u32 _labelTileBufferSize = 0;
    u32 _labelNewStringWidth = 0;
    std::unique_ptr<char16_t[]> _labelTextBuffer;
    std::unique_ptr<u8[]> _labelTileBuffer;
    bool _labelTileBufferUpdated = false;
    u32 _labelVramOffset = 0;
    vu16* _labelVramAddress = nullptr;

    Animator<int> _alphaAnimator;
    Animator<int> _yAnimator;
    State _state = State::Hidden;
    int _holdFrames = 0;

    char _currentName[kMaxNameLength + 1] = { 0 };
};
