#pragma once
#include "../../views/IconGridItemView.h"

class CustomThemeInfo;

class CustomIconGridItemView : public IconGridItemView
{
    SHARED_ONLY(CustomIconGridItemView)

public:
    void Draw(GraphicsContext& graphicsContext) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x + 2, _position.y + 2, 40, 40);
    }

private:
    const CustomThemeInfo* _customThemeInfo;
    u32 _texVramOffset;
    u32 _plttVramOffset;
    u32 _selectedTexVramOffset;
    u32 _selectedPlttVramOffset;

    CustomIconGridItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel, const CustomThemeInfo* customThemeInfo,
        u32 texVramOffset, u32 plttVramOffset, u32 selectedTexVramOffset, u32 selectedPlttVramOffset)
        : IconGridItemView(std::move(viewModel)), _customThemeInfo(customThemeInfo), _texVramOffset(texVramOffset), _plttVramOffset(plttVramOffset)
        , _selectedTexVramOffset(selectedTexVramOffset), _selectedPlttVramOffset(selectedPlttVramOffset) { }
};