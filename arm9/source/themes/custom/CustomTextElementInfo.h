#pragma once
#include "core/math/Point.h"
#include "core/math/Rgb.h"
#include "themes/FontType.h"

class CustomTopTextElementInfo
{
public:
    CustomTopTextElementInfo(const Point& position, u32 width, const Rgb8& textColor, const Rgb8& blendColor, FontType fontType)
        : _position(position), _width(width), _textColor(textColor), _blendColor(blendColor), _fontType(fontType) { }

    const Point& GetPosition() const { return _position; }
    const u32 GetWidth() const { return _width; }
    const Rgb8& GetTextColor() const { return _textColor; }
    const Rgb8& GetBlendColor() const { return _blendColor; }
    FontType GetFontType() const { return _fontType; }

private:
    Point _position;
    u32 _width;
    Rgb8 _textColor;
    Rgb8 _blendColor;
    FontType _fontType;
};
