#pragma once
#include "core/math/Point.h"
#include "core/math/Rgb.h"
#include "themes/FontType.h"

class CustomTopTextElementInfo
{
public:
    CustomTopTextElementInfo(
        const Point& position, u32 width, const Rgb8& textColor, const Rgb8& blendColor,
        FontType fontType, bool visible = true, bool marquee = true)
        : _position(position), _width(width), _textColor(textColor), _blendColor(blendColor)
        , _fontType(fontType), _visible(visible), _marquee(marquee) { }

    const Point& GetPosition() const { return _position; }
    const u32 GetWidth() const { return _width; }
    const Rgb8& GetTextColor() const { return _textColor; }
    const Rgb8& GetBlendColor() const { return _blendColor; }
    FontType GetFontType() const { return _fontType; }
    bool IsVisible() const { return _visible; }
    bool IsMarqueeEnabled() const { return _marquee; }

private:
    Point _position;
    u32 _width;
    Rgb8 _textColor;
    Rgb8 _blendColor;
    FontType _fontType;
    bool _visible;
    bool _marquee;
};
