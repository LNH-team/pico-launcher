#pragma once
#include <algorithm>
#include "gui/views/View.h"
#include "gui/views/Label2DView.h"
#include "gui/materialDesign.h"
#include "themes/IFontRepository.h"
#include "core/math/Rgb.h"

class MaterialColorScheme;
class IVramManager;

#define CHIP_VIEW_MIN_WIDTH     80
#define CHIP_VIEW_MAX_WIDTH     140

class ChipView : public View
{
public:
    class VramToken
    {
        u32 _vramOffset;
    public:
        VramToken()
            : _vramOffset(0) { }

        explicit VramToken(u32 offset)
            : _vramOffset(offset) { }

        constexpr u32 GetVramOffset() const { return _vramOffset; }
    };

    explicit ChipView(md::sys::color backgroundColor, const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository)
        : _vramOffset(0), _isSelected(false), _backgroundColor(backgroundColor)
        , _label(CHIP_VIEW_MAX_WIDTH - 20, 16, 30, fontRepository->GetFont(FontType::Medium10))
        , _secondaryLabel(CHIP_VIEW_MAX_WIDTH - 20, 12, 30, fontRepository->GetFont(FontType::Medium7_5))
        , _iconVramOffset(0xFFFFFFFF), _materialColorScheme(materialColorScheme) { }

    void InitVram(const VramContext& vramContext) override { _label.InitVram(vramContext); _secondaryLabel.InitVram(vramContext); }

    void SetText(const char16_t* text) { _label.SetText(text); }
    void SetText(const char16_t* text, u32 length) { _label.SetText(text, length); }
    QueueTask<void> SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text) { return _label.SetTextAsync(taskQueue, text); }
    QueueTask<void> SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text, u32 length) { return _label.SetTextAsync(taskQueue, text, length); }

    void Draw(GraphicsContext& graphicsContext) override;

    void SetGraphics(const VramToken& vramToken)
    {
        _vramOffset = vramToken.GetVramOffset();
    }

    void SetSelected(bool selected)
    {
        _isSelected = selected;
    }

    void SetIcon(bool enabled, u32 vramOffset)
    {
        _iconVramOffset = enabled ? vramOffset : 0xFFFFFFFF;
    }

    void SetCenteredText(bool centered)
    {
        _centeredText = centered;
    }

    void SetSecondaryText(const char16_t* text)
    {
        _hasSecondaryText = text != nullptr && text[0] != 0;
        if (_hasSecondaryText)
            _secondaryLabel.SetText(text);
        else
            _secondaryLabel.SetText(u"");
    }

    void SetFixedWidth(int width)
    {
        _fixedWidth = width;
    }

    void SetMinWidth(int minWidth)
    {
        _minWidth = std::clamp(minWidth, 64, CHIP_VIEW_MAX_WIDTH);
    }

    int GetWidth() const
    {
        if (_fixedWidth > 0)
            return std::max(_fixedWidth, _minWidth);

        int width;
        int primaryTextWidth = _label.GetStringWidth();
        if (_hasSecondaryText)
            primaryTextWidth = std::max(primaryTextWidth, static_cast<int>(_secondaryLabel.GetStringWidth()));

        if (_iconVramOffset == 0xFFFFFFFF)
            width = 10 + primaryTextWidth + 10;
        else
            width = 22 + primaryTextWidth + 10;
        width = std::clamp(width, _minWidth, CHIP_VIEW_MAX_WIDTH);
        return width;
    }

    int GetHeight() const { return _hasSecondaryText ? 28 : 20; }
    void VBlank() override { _label.VBlank(); _secondaryLabel.VBlank(); }

    static VramToken UploadGraphics(IVramManager& vramManager);

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, GetWidth(), GetHeight());
    }

private:
    u32 _vramOffset;
    bool _isSelected;
    md::sys::color _backgroundColor;
    Label2DView _label;
    Label2DView _secondaryLabel;
    u32 _iconVramOffset;
    const MaterialColorScheme* _materialColorScheme;
    bool _centeredText = false;
    int _fixedWidth = -1;
    int _minWidth = CHIP_VIEW_MIN_WIDTH;
    bool _hasSecondaryText = false;

    void DrawIcon(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& fgColor);
};