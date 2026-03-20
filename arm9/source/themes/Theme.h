#pragma once
#include "fat/ff.h"
#include "core/String.h"
#include "ITheme.h"

class Theme : public ITheme
{
public:
    bool OpenThemeFile(File& file, const TCHAR* subPath) const override;
    void LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext) override { };
    const MaterialColorScheme& GetMaterialColorScheme() const override { return _materialColorScheme; }
    void SetDarkMode(bool darkMode) override;

protected:
    Theme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode);

    MaterialColorScheme _materialColorScheme;

private:
    String<TCHAR, 64> _folderName;
    Rgb<8, 8, 8> _primaryColor;
};
