#pragma once
#include "core/math/Point.h"
#include "core/math/Rgb.h"

class CustomBottomIconInfo
{
public:
    CustomBottomIconInfo(const Rgb8& blendColor, bool visible = true)
        : _blendColor(blendColor), _visible(visible) { }

    const Rgb8& GetBlendColor() const { return _blendColor; }
    bool IsVisible() const { return _visible; }

private:
    Rgb8 _blendColor;
    bool _visible;
};
