#include "common.h"
#include "material/MaterialTheme.h"
#include "custom/CustomTheme.h"
#include "ThemeFactory.h"

std::unique_ptr<ITheme> ThemeFactory::CreateFromThemeInfo(const ThemeInfo* themeInfo) const
{
    return CreateFromThemeInfo(themeInfo, themeInfo->GetIsDarkTheme());
}

std::unique_ptr<ITheme> ThemeFactory::CreateFromThemeInfo(const ThemeInfo* themeInfo, bool darkModeOverride) const
{
    switch (themeInfo->GetType())
    {
        case ThemeType::Material:
        {
            return std::make_unique<MaterialTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                darkModeOverride);
        }
        case ThemeType::Custom:
        {
            return std::make_unique<CustomTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                darkModeOverride);
        }
        default:
        {
            return nullptr;
        }
    }
}