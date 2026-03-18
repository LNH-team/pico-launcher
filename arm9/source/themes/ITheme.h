#pragma once
#include <memory>
#include "romBrowser/Theme/IThemeFileIconFactory.h"
#include "romBrowser/Theme/IRomBrowserViewFactory.h"
#include "IFontRepository.h"
#include "background/IThemeBackground.h"
#include "material/MaterialColorScheme.h"
#include "gui/font/nitroFont2.h"
#include "fat/File.h"

class VramContext;

class ITheme
{
public:
    virtual ~ITheme() = 0;

    virtual const IFontRepository* GetFontRepository() const = 0;
    virtual const IThemeFileIconFactory* GetThemeFileIconFactory() const = 0;
    virtual const IRomBrowserViewFactory* GetRomBrowserViewFactory() const = 0;

    virtual std::unique_ptr<IThemeBackground> CreateRomBrowserTopBackground() const = 0;
    virtual std::unique_ptr<IThemeBackground> CreateRomBrowserBottomBackground() const = 0;

    virtual void LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext) = 0;

    virtual bool OpenThemeFile(File& file, const TCHAR* subPath) const = 0;

    virtual const MaterialColorScheme& GetMaterialColorScheme() const = 0;

    virtual void SetDarkMode(bool darkMode) = 0;
};

inline ITheme::~ITheme() { }
