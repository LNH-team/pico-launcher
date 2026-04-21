#pragma once
#include "gui/views/LabelView.h"
#include "romBrowser/views/BannerListItemView.h"

class MaterialColorScheme;
class IFontRepository;
class CustomThemeInfo;

class CustomBannerListItemView : public BannerListItemView
{
    SHARED_ONLY(CustomBannerListItemView)

public:
    void Draw(GraphicsContext& graphicsContext) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 203, 44);
    }

private:
    const CustomThemeInfo* _customThemeInfo;
    const MaterialColorScheme* _materialColorScheme;
    u32 _texVramOffset = 0;
    u32 _plttVramOffset = 0;
    u32 _selectedTexVramOffset = 0;
    u32 _selectedPlttVramOffset = 0;

    CustomBannerListItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel, const CustomThemeInfo* customThemeInfo,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        u32 texVramOffset, u32 plttVramOffset, u32 selectedTexVramOffset, u32 selectedPlttVramOffset,
        VBlankTextureLoader* vblankTextureLoader);
};
