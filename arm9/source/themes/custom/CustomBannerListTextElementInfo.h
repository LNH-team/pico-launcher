#pragma once
#include "core/math/Point.h"
#include "core/math/Rgb.h"
#include "themes/FontType.h"

class CustomBannerListTextElementInfo
{
public:
    CustomBannerListTextElementInfo(const Rgb8& textColor, FontType fontType)
        : _textColor(textColor), _fontType(fontType) { }

    const Rgb8& GetTextColor() const { return _textColor; }
    FontType GetFontType() const { return _fontType; }

private:
    Rgb8 _textColor;
    FontType _fontType;
};
