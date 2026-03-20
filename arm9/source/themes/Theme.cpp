#include "common.h"
#include "core/mini-printf.h"
#include "material/MaterialColorSchemeFactory.h"
#include "Theme.h"

Theme::Theme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode)
    : _folderName(folderName)
    , _primaryColor(primaryColor)
{
    MaterialColorSchemeFactory::FromPrimaryColor(primaryColor, darkMode, _materialColorScheme);
}

void Theme::SetDarkMode(bool darkMode)
{
    MaterialColorSchemeFactory::FromPrimaryColor(_primaryColor, darkMode, _materialColorScheme);
}

bool Theme::OpenThemeFile(File& file, const TCHAR* subPath) const
{
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/%s", _folderName.GetString(), subPath);
    return file.Open(pathBuffer, FA_OPEN_EXISTING | FA_READ) == FR_OK;
}
