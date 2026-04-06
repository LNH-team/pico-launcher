#pragma once
#include "core/math/Point.h"

class CustomTopCoverInfo
{
public:
    CustomTopCoverInfo(const Point& position, bool visible = true)
        : _position(position), _visible(visible) { }

    const Point& GetPosition() const { return _position; }
    bool IsVisible() const { return _visible; }

private:
    Point _position;
    bool _visible;
};
