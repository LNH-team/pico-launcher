#pragma once
#include <memory>
#include "ITheme.h"
#include "ThemeInfo.h"
#include "services/settings/IAppSettingsService.h"

class ThemeFactory
{
public:
    std::unique_ptr<ITheme> CreateFromThemeInfo(const ThemeInfo* themeInfo) const;
    std::unique_ptr<ITheme> CreateFromThemeInfo(const ThemeInfo* themeInfo, bool darkModeOverride) const;
};
