#pragma once
#include "InputProvider.h"

/// @brief Input provider that wraps another input provider and adds key repetition on top of it.
class InputRepeater : public InputProvider
{
public:
    InputRepeater(InputProvider* inputProvider, InputKey repeatMask, u16 firstRepeatDelay, u16 nextRepeatDelay)
        : _inputProvider(inputProvider), _state(State::Idle)
        , _frameCounter(0), _repeatMask(repeatMask & ~InputKey::Touch)
        , _firstRepeatDelayFrames(firstRepeatDelay)
        , _nextRepeatDelayFrames(nextRepeatDelay) { }

    void Update() override;
    void Reset() override;

private:
    enum class State
    {
        Idle,
        FirstRepeat,
        NextRepeat
    };

    InputProvider* _inputProvider;
    State _state;
    u16 _frameCounter;
    InputKey _repeatMask;
    u16 _firstRepeatDelayFrames;
    u16 _nextRepeatDelayFrames;
};
