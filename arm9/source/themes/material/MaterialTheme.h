#pragma once
#include <libtwl/gfx/gfxPalette.h>
#include "../Theme.h"
#include "MaterialColorScheme.h"
#include "core/math/Rgb.h"
#include "romBrowser/Theme/Material/MaterialThemeFileIconFactory.h"
#include "romBrowser/Theme/Material/MaterialRomBrowserViewFactory.h"
#include "MaterialMainBackground.h"
#include "MaterialSubBackground.h"
#include "../DefaultFontRepository.h"

class MaterialTheme : public Theme
{
    MaterialThemeFileIconFactory _themeFileIconFactory;
    MaterialRomBrowserViewFactory _romBrowserViewFactory;
    DefaultFontRepository _fontRepository;
public:
    MaterialTheme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode)
        : Theme(folderName, primaryColor, darkMode)
        , _themeFileIconFactory(&_materialColorScheme, &_fontRepository)
        , _romBrowserViewFactory(&_materialColorScheme, &_fontRepository, darkMode) { }

    const IFontRepository* GetFontRepository() const override
    {
        return &_fontRepository;
    }

    const IThemeFileIconFactory* GetThemeFileIconFactory() const override
    {
        return &_themeFileIconFactory;
    }

    const IRomBrowserViewFactory* GetRomBrowserViewFactory() const override
    {
        return &_romBrowserViewFactory;
    }

    std::unique_ptr<IThemeBackground> CreateRomBrowserTopBackground() const override
    {
        return std::make_unique<MaterialSubBackground>(&_materialColorScheme);
    }

    std::unique_ptr<IThemeBackground> CreateRomBrowserBottomBackground() const override
    {
        return std::make_unique<MaterialMainBackground>(&_materialColorScheme);
    }

    void LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext) override;
};
