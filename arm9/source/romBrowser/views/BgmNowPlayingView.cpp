#include "common.h"
#include <algorithm>
#include <string.h>
#include "core/StringUtil.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/OamBuilder.h"
#include "gui/OamManager.h"
#include "gui/palette/GradientPalette.h"
#include "gui/palette/DirectPalette.h"
#include "core/math/ColorConverter.h"
#include "gui/materialDesign.h"
#include "chipFilled.h"
#include "BgmNowPlayingView.h"

BgmNowPlayingView::BgmNowPlayingView(const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _materialColorScheme(materialColorScheme)
    , _font(fontRepository->GetFont(FontType::Medium10))
{
    _labelTextBuffer = std::make_unique_for_overwrite<char16_t[]>(kMaxNameLength + 1);
    _labelTextBuffer[0] = 0;

    _labelActualWidth = (kLabelMaxWidth + 31) & ~31;
    _labelActualHeight = (kLabelHeight + 15) & ~15;
    _labelTileBufferSize = (_labelActualWidth * _labelActualHeight) >> 1;
    _labelTileBuffer = std::make_unique_for_overwrite<u8[]>(_labelTileBufferSize);
    UpdateLabelTileBuffer();
}

void BgmNowPlayingView::InitVram(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _chipVramOffset = objVramManager->Alloc(chipFilledTilesLen);
        dma_ntrCopy32(3, chipFilledTiles, objVramManager->GetVramAddress(_chipVramOffset), chipFilledTilesLen);

        _labelVramOffset = objVramManager->Alloc(_labelTileBufferSize);
        _labelVramAddress = objVramManager->GetVramAddress(_labelVramOffset);
    }
}

void BgmNowPlayingView::SetBgmName(const char* name)
{
    if (!name || name[0] == 0)
    {
        _currentName[0] = 0;
        _state = State::Hidden;
        return;
    }

    if (strncmp(_currentName, name, sizeof(_currentName)) == 0 && _state != State::Hidden)
        return;

    StringUtil::Copy(_currentName, name, sizeof(_currentName));
    StringUtil::Copy(_labelTextBuffer.get(), name, kMaxNameLength + 1);

    UpdateBaseWidth();
    UpdateLabelTileBuffer();

    _state = State::Hold;
    _holdFrames = kHoldFrames;
    _alphaAnimator = Animator<int>(100, 100, md::sys::motion::duration::short1, &md::sys::motion::easing::standardDecelerate);
    _yAnimator = Animator<int>(0, 0, md::sys::motion::duration::short1, &md::sys::motion::easing::standardDecelerate);
}

void BgmNowPlayingView::StartFadeOut()
{
    _state = State::FadeOut;
    _alphaAnimator.Goto(0, md::sys::motion::duration::short3, &md::sys::motion::easing::standardAccelerate);
    _yAnimator.Goto(-8, md::sys::motion::duration::short3, &md::sys::motion::easing::standardAccelerate);
}

void BgmNowPlayingView::UpdateBaseWidth()
{
    u32 textWidth = 0;
    u32 textHeight = 0;
    nft2_measureString(_font, _labelTextBuffer.get(), textWidth, textHeight);

    int targetWidth = (int)textWidth + kPaddingX * 2;
    _baseWidth = std::clamp(targetWidth, kMinBaseWidth, kMaxBaseWidth);
    u32 rawWidth = (u32)std::min(kLabelMaxWidth, _baseWidth - kPaddingX * 2);
    _labelRenderWidth = (rawWidth + 31) & ~31;   
}

void BgmNowPlayingView::UpdateLabelTileBuffer()
{
    memset(_labelTileBuffer.get(), 0, _labelTileBufferSize);
    if (_labelTextBuffer[0] != 0)
    {
        u32 measuredWidth = 0;
        u32 measuredHeight = 0;
        nft2_measureString(_font, _labelTextBuffer.get(), measuredWidth, measuredHeight);

        nft2_string_render_params_t renderParams;
        renderParams.x = 0;        
        renderParams.y = 0;
        renderParams.width = _labelRenderWidth;
        renderParams.height = kLabelHeight;
        renderParams.a5i3 = false;

        nft2_renderStringEllipsis(
            _font, _labelTextBuffer.get(), _labelTileBuffer.get(),
            _labelActualWidth, &renderParams, u" ... ");
        _labelNewStringWidth = renderParams.textWidth;
    }
    else
    {
        _labelNewStringWidth = 0;
    }

    _labelTileBufferUpdated = true;
}

void BgmNowPlayingView::Update()
{
    switch (_state)
    {
        case State::FadeIn:
        {
            bool done = _alphaAnimator.Update();
            _yAnimator.Update();
            if (done)
                _state = State::Hold;
            break;
        }
        case State::Hold:
        {
            if (_holdFrames > 0)
                _holdFrames--;
            if (_holdFrames <= 0)
                StartFadeOut();
            break;
        }
        case State::FadeOut:
        {
            _alphaAnimator.Update();
            _yAnimator.Update();
            if (_alphaAnimator.IsFinished())
                _state = State::Hidden;
            break;
        }
        case State::Hidden:
        default:
            break;
    }
}

void BgmNowPlayingView::Draw(GraphicsContext& graphicsContext)
{
    if (_state == State::Hidden)
        return;

    int alpha = _alphaAnimator.GetValue();
    if (alpha <= 0)
        return;

    int baseX = _position.x + (kScreenWidth - _baseWidth) / 2;
    int baseY = _position.y + _yAnimator.GetValue() + kVerticalOffset;

    Rectangle bounds(baseX, baseY, _baseWidth, kBaseHeight);
    if (!graphicsContext.IsVisible(bounds))
        return;

    Rgb<8,8,8> fadeBg(
        (u8)BgmNowPlayingView::kBannerR,
        (u8)BgmNowPlayingView::kBannerG,
        (u8)BgmNowPlayingView::kBannerB);
    Rgb<8,8,8> fadeText(
        (u8)BgmNowPlayingView::kTextR,
        (u8)BgmNowPlayingView::kTextG,
        (u8)BgmNowPlayingView::kTextB);

    u16 chipPltt[16];
    std::fill_n(chipPltt, 16, ColorConverter::ToGBGR565(fadeBg.Clamped()));

    u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(chipPltt), baseY, baseY + kBaseHeight);

    auto oams = graphicsContext.GetOamManager().AllocOams(2);
    OamBuilder::OamWithSize<64, 32>(baseX, baseY, _chipVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[0]);
    OamBuilder::OamWithSize<64, 32>(baseX + _baseWidth - 64, baseY, _chipVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .WithHFlip()
        .Build(oams[1]);

    int labelX = baseX + kPaddingX;
    int labelY = baseY + kPaddingY;

    u32 hCellCount = _labelActualWidth >> 5;
    u32 vCellCount = _labelActualHeight >> 4;
    u32 cellCount = hCellCount * vCellCount;

    u32 labelPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(fadeBg, fadeText), labelY, labelY + kLabelHeight);

    auto labelOams = graphicsContext.GetOamManager().AllocOams(cellCount);
    u32 i = 0;
    for (u32 y = 0; y < vCellCount; y++)
    {
        for (u32 x = 0; x < hCellCount; x++)
        {
            OamBuilder::OamWithSize<32, 16>(
                    labelX + x * 32,
                    labelY + y * 16,
                    (_labelVramOffset + i * 256) >> 7)
                .WithPalette16(labelPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(labelOams[i]);
            i++;
        }
    }
}

void BgmNowPlayingView::VBlank()
{
    if (_labelTileBufferUpdated && _labelVramAddress)
    {
        memcpy((void*)_labelVramAddress, _labelTileBuffer.get(), _labelTileBufferSize);
        _labelTileBufferUpdated = false;
    }
}
