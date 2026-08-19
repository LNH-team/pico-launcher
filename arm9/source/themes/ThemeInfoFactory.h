#pragma once
#include <memory>
#include "ThemeInfo.h"

class ThemeInfoFactory
{
public:
    std::unique_ptr<ThemeInfo> CreateFromThemeFolder(const TCHAR* folderName) const;

    std::unique_ptr<ThemeInfo> CreateFallbackTheme() const
    {
        return std::make_unique<ThemeInfo>("", ThemeType::Material, "Fallback", "", "", Rgb<8,8,8>(138, 217, 255), false,
            IconCoverMode::Disabled, Rgb<8,8,8>(0xFF, 0xFF, 0xFF));
    }
};
