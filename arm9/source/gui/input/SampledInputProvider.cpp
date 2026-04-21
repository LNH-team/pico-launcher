#include "common.h"
#include "SampledInputProvider.h"

void SampledInputProvider::Update()
{
    InputKey curKeys = _currentKeys;
    InputKey trig = InputKey::None;
    InputKey rel = InputKey::None;

    Point touchPoint(0, 0);
    int touchPointCount = 0;
    while (_inputBufferReadPtr != _inputBufferWritePtr)
    {
        InputKey nextKeys = _keyInputBuffer[_inputBufferReadPtr];
        trig |= (nextKeys ^ curKeys) & nextKeys;
        rel |= (nextKeys ^ curKeys) & curKeys;
        curKeys = nextKeys;
        if ((nextKeys & InputKey::Touch) != InputKey::None)
        {
            touchPoint.x += _touchInputBuffer[_inputBufferReadPtr].x;
            touchPoint.y += _touchInputBuffer[_inputBufferReadPtr].y;
            touchPointCount++;
        }
        else
        {
            touchPoint = Point(0, 0);
            touchPointCount = 0;
        }
        _inputBufferReadPtr = (_inputBufferReadPtr + 1) & 3;
    }

    _triggeredKeys = trig;
    _releasedKeys = rel;
    _currentKeys = curKeys;
    if (touchPointCount == 0)
    {
        if ((_triggeredKeys & InputKey::Touch) != InputKey::None ||
            (_currentKeys & InputKey::Touch) != InputKey::None)
        {
            _releasedKeys = _releasedKeys | InputKey::Touch;
        }
        _triggeredKeys = _triggeredKeys & ~InputKey::Touch;
        _currentKeys = _currentKeys & ~InputKey::Touch;
    }
    else
    {
        _currentTouchPoint.x = touchPoint.x / touchPointCount;
        _currentTouchPoint.y = touchPoint.y / touchPointCount;
    }
}
