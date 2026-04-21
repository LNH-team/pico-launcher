#include "common.h"
#include "InputRepeater.h"

void InputRepeater::Update()
{
    _inputProvider->Update();
    InputKey curKeys = _inputProvider->GetCurrentKeys();
    InputKey repKeys = InputKey::None;
    if (_state == State::Idle)
    {
        if (static_cast<bool>(curKeys & _repeatMask))
        {
            _state = State::FirstRepeat;
            _frameCounter = 0;
            repKeys = curKeys & _repeatMask;
        }
    }
    else if (_state == State::FirstRepeat)
    {
        if (static_cast<bool>(curKeys & _repeatMask))
        {
            if (++_frameCounter >= _firstRepeatDelayFrames)
            {
                _state = State::NextRepeat;
                _frameCounter = 0;
                repKeys = curKeys & _repeatMask;
            }
        }
        else
        {
            _state = State::Idle;
        }
    }
    else if (_state == State::NextRepeat)
    {
        if (static_cast<bool>(curKeys & _repeatMask))
        {
            if (++_frameCounter >= _nextRepeatDelayFrames)
            {
                _frameCounter = 0;
                repKeys = curKeys & _repeatMask;
            }
        }
        else
        {
            _state = State::Idle;
        }
    }

    InputKey lastRepKeys = _currentKeys & _repeatMask;
    _currentKeys = curKeys | repKeys;
    _triggeredKeys = _inputProvider->GetTriggeredKeys() | repKeys;
    _releasedKeys = _inputProvider->GetReleasedKeys() | (lastRepKeys & (lastRepKeys ^ repKeys));
    Point touchPoint;
    if (_inputProvider->GetCurrentTouchPoint(touchPoint))
    {
        _currentTouchPoint = touchPoint;
    }
    else
    {
        _currentTouchPoint = Point(0, 0);
    }
}

void InputRepeater::Reset()
{
    InputProvider::Reset();
    _inputProvider->Reset();
    _state = State::Idle;
    _frameCounter = 0;
}
