#pragma once
#include "gui/views/View.h"
#include "gui/views/Label2DView.h"
#include "gui/materialDesign.h"
#include "themes/IFontRepository.h"
#include "core/math/Rgb.h"

class MaterialColorScheme;
class IVramManager;

#define CHIP_VIEW_MIN_WIDTH     53
#define CHIP_VIEW_MAX_WIDTH     96

class ChipView : public View
{
    SHARED_ONLY(ChipView)

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

    void InitVram(const VramContext& vramContext) override { _label->InitVram(vramContext); }

    void SetText(const char16_t* text) { _label->SetText(text); }
    void SetText(const char16_t* text, u32 length) { _label->SetText(text, length); }
    QueueTask<void> SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text) { return _label->SetTextAsync(taskQueue, text); }
    QueueTask<void> SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text, u32 length) { return _label->SetTextAsync(taskQueue, text, length); }

    void Draw(GraphicsContext& graphicsContext) override;

    void VBlank() override { _label->VBlank(); }

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

    int GetWidth() const
    {
        int width;
        if (_iconVramOffset == 0xFFFFFFFF)
        {
            width = 10 + _label->GetStringWidth() + 10;
        }
        else
        {
            width = 22 + _label->GetStringWidth() + 10;
        }
        width = std::clamp(width, CHIP_VIEW_MIN_WIDTH, CHIP_VIEW_MAX_WIDTH);
        return width;
    }

    int GetHeight() const { return 20; }

    static VramToken UploadGraphics(IVramManager& vramManager);

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, GetWidth(), GetHeight());
    }

private:
    u32 _vramOffset;
    bool _isSelected;
    md::sys::color _backgroundColor;
    SharedPtr<Label2DView> _label;
    u32 _iconVramOffset;
    const MaterialColorScheme* _materialColorScheme;

    ChipView(md::sys::color backgroundColor, const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository)
        : _vramOffset(0), _isSelected(false), _backgroundColor(backgroundColor)
        , _label(Label2DView::CreateShared(CHIP_VIEW_MAX_WIDTH - 20, 16, 30, fontRepository->GetFont(FontType::Medium10)))
        , _iconVramOffset(0xFFFFFFFF), _materialColorScheme(materialColorScheme) { }

    void DrawIcon(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& fgColor);
};