#pragma once
#include "core/math/Point.h"
#include "core/math/Rgb.h"

class CustomTopIconInfo
{
public:
    CustomTopIconInfo(const Point& position, const Rgb8& blendColor, bool visible = true)
        : _position(position), _blendColor(blendColor), _visible(visible) { }

    const Point& GetPosition() const { return _position; }
    const Rgb8& GetBlendColor() const { return _blendColor; }
    bool IsVisible() const { return _visible; }

private:
    Point _position;
    Rgb8 _blendColor;
    bool _visible;
};
