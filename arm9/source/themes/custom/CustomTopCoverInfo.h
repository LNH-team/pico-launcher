#pragma once
#include "core/math/Point.h"

class CustomTopCoverInfo
{
public:
    CustomTopCoverInfo(const Point& position)
        : _position(position) { }

    const Point& GetPosition() const { return _position; }

private:
    Point _position;
};
