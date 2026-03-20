#include "common.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "core/StringUtil.h"
#include "gui/palette/GradientPalette.h"
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "CheatListItemView.h"

#define ICON_X             4
#define ICON_Y             4

#define NAME_LABEL_X       24
#define NAME_LABEL_Y       5
#define NAME_LABEL_WIDTH    200
#define NAME_SCROLL_SEPARATOR "   "

#define NAME_SCROLL_START_PAUSE_FRAMES 120
#define NAME_SCROLL_CYCLE_PAUSE_FRAMES 120
#define NAME_SCROLL_SPEED_Q8           96
#define NAME_SCROLL_SPEED_FAST_Q8      384

bool CheatListItemView::sFastScrollEnabled = false;
u32 CheatListItemView::sForceScrollStartRequestId = 0;

CheatListItemView::CheatListItemView(const VramOffsets& vramOffsets,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _nameFont(fontRepository->GetFont(FontType::Regular10))
    , _nameLabel(NAME_LABEL_WIDTH, 16, 320, _nameFont)
    , _vramOffsets(vramOffsets)
    , _materialColorScheme(materialColorScheme)
{
    _nameLabel.SetEllipsis(false);
    ResetNameScroll();
    AddChildTail(&_nameLabel);
}

void CheatListItemView::ResetNameScroll()
{
    _nameScrollPhase = NameScrollPhase::PauseAtStart;
    _nameScrollPauseFrames = NAME_SCROLL_START_PAUSE_FRAMES;
    _nameScrollOffsetQ8 = 0;
    _nameLabel.SetTextOffsetX(0);
}

void CheatListItemView::SetBaseName(const char* name)
{
    StringUtil::Copy(_baseName, name, sizeof(_baseName));
    _nameScrollPrepared = false;
    _nameScrollCycleQ8 = 0;
    _handledForceScrollStartRequestId = sForceScrollStartRequestId;
    _nameLabel.SetText(_baseName);
    ResetNameScroll();
}

void CheatListItemView::PrepareNameScrollIfNeeded()
{
    if (_nameScrollPrepared)
    {
        return;
    }

    _nameScrollPrepared = true;
    _nameScrollCycleQ8 = 0;

    char16_t baseName16[96];
    StringUtil::Copy(baseName16, _baseName, sizeof(baseName16) / sizeof(baseName16[0]));

    u32 baseWidth = 0;
    u32 baseHeight = 0;
    nft2_measureString(_nameFont, baseName16, baseWidth, baseHeight);
    if ((int)baseWidth <= NAME_LABEL_WIDTH)
    {
        _nameLabel.SetText(_baseName);
        return;
    }

    char16_t separator16[8];
    StringUtil::Copy(separator16, NAME_SCROLL_SEPARATOR, sizeof(separator16) / sizeof(separator16[0]));
    u32 separatorWidth = 0;
    u32 separatorHeight = 0;
    nft2_measureString(_nameFont, separator16, separatorWidth, separatorHeight);

    char scrollerText[320];
    scrollerText[0] = 0;
    strlcat(scrollerText, _baseName, sizeof(scrollerText));
    strlcat(scrollerText, NAME_SCROLL_SEPARATOR, sizeof(scrollerText));
    strlcat(scrollerText, _baseName, sizeof(scrollerText));
    strlcat(scrollerText, NAME_SCROLL_SEPARATOR, sizeof(scrollerText));
    strlcat(scrollerText, _baseName, sizeof(scrollerText));
    _nameLabel.SetText(scrollerText);

    _nameScrollCycleQ8 = ((int)baseWidth + (int)separatorWidth) << 8;
}

void CheatListItemView::UpdateNameScroll()
{
    PrepareNameScrollIfNeeded();
    if (_nameScrollCycleQ8 <= 0)
    {
        if (_nameScrollOffsetQ8 != 0)
        {
            ResetNameScroll();
        }
        return;
    }

    if (!IsFocused())
    {
        if (_nameScrollOffsetQ8 != 0 || _nameScrollPauseFrames != NAME_SCROLL_START_PAUSE_FRAMES)
        {
            ResetNameScroll();
        }
        return;
    }

    if (IsFocused() && _handledForceScrollStartRequestId != sForceScrollStartRequestId)
    {
        _handledForceScrollStartRequestId = sForceScrollStartRequestId;
        _nameScrollPauseFrames = 0;
        _nameScrollPhase = NameScrollPhase::Scrolling;
    }

    if (_nameScrollPhase == NameScrollPhase::PauseAtStart)
    {
        if (_nameScrollPauseFrames > 0)
        {
            _nameScrollPauseFrames--;
            return;
        }

        _nameScrollPhase = NameScrollPhase::Scrolling;
    }

    const int speedQ8 = sFastScrollEnabled ? NAME_SCROLL_SPEED_FAST_Q8 : NAME_SCROLL_SPEED_Q8;
    _nameScrollOffsetQ8 += speedQ8;

    if (_nameScrollOffsetQ8 >= _nameScrollCycleQ8)
    {
        _nameScrollOffsetQ8 = 0;
        _nameLabel.SetTextOffsetX(0);
        _nameScrollPhase = NameScrollPhase::PauseAtStart;
        _nameScrollPauseFrames = NAME_SCROLL_CYCLE_PAUSE_FRAMES;
        return;
    }

    _nameLabel.SetTextOffsetX(-(_nameScrollOffsetQ8 >> 8));
}

void CheatListItemView::Update()
{
    _nameLabel.SetPosition(_position.x + NAME_LABEL_X, _position.y + NAME_LABEL_Y);
    UpdateNameScroll();
    if (_cheatEntry != nullptr && !_cheatEntry->IsCheatCategory())
    {
        _iconVramOffset = _cheatEntry->GetIsCheatActive()
            ? _vramOffsets.checkboxCheckedIconVramOffset
            : _vramOffsets.checkboxUncheckedIconVramOffset;
    }
    ViewContainer::Update();
}

void CheatListItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
    {
        return;
    }

    auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
    if (IsFocused())
    {
        auto selectorFullColor = _materialColorScheme->GetColor(md::sys::color::onSurface);
        backColor = RgbMixer::Lerp(backColor, selectorFullColor, 10, 100);
    }

    _nameLabel.SetBackgroundColor(backColor);
    _nameLabel.SetForegroundColor(_materialColorScheme->onSurface);

    if (IsFocused())
    {
        u32 selectorPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, backColor),
            _position.y, _position.y + 24);
        auto selectorOam = graphicsContext.GetOamManager().AllocOams(4);
        OamBuilder::OamWithSize<64, 32>(_position.x, _position.y, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[0]);
        OamBuilder::OamWithSize<64, 32>(_position.x + 64, _position.y, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[1]);
        OamBuilder::OamWithSize<64, 32>(_position.x + 2 * 64, _position.y, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[2]);
        OamBuilder::OamWithSize<64, 32>(_position.x + 2 * 64 + 32, _position.y, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[3]);
    }

    ViewContainer::Draw(graphicsContext);

    if (graphicsContext.IsVisible(Rectangle(_position.x + ICON_X, _position.y + ICON_Y, 16, 16)))
    {
        u32 iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, _materialColorScheme->primary),
            _position.y + ICON_Y, _position.y + ICON_Y + 16);
        auto iconOam = graphicsContext.GetOamManager().AllocOams(1);
        OamBuilder::OamWithSize<16, 16>(_position.x + ICON_X, _position.y + ICON_Y, _iconVramOffset >> 7)
            .WithPalette16(iconPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(iconOam[0]);
    }
}
