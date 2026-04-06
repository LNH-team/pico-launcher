#pragma once
#include "core/math/Point.h"
#include "core/math/Rgb.h"
#include "themes/FontType.h"

class CustomBannerListTextElementInfo
{
public:
    CustomBannerListTextElementInfo(const Rgb8& textColor, FontType fontType, bool visible = true)
        : _textColor(textColor), _fontType(fontType), _visible(visible) { }

    const Rgb8& GetTextColor() const { return _textColor; }
    FontType GetFontType() const { return _fontType; }
    bool IsVisible() const { return _visible; }

private:
    Rgb8 _textColor;
    FontType _fontType;
    bool _visible;
};
