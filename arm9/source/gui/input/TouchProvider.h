#pragma once
#include "common.h"
#include "TouchEvent.h"
#include "sharedMemory.h"

/// @brief Processes raw touch screen data into touch events with gesture detection.
/// Sample() must be called in VBlank, Update() in the main loop.
class TouchProvider
{
public:
    TouchProvider();

    /// @brief Samples the touch screen state. Call in VBlank.
    void Sample();

    /// @brief Updates the touch provider and generates events. Call in main loop.
    void Update();

    /// @brief Resets the touch provider state.
    void Reset();

    /// @brief Returns whether a touch event occurred this frame.
    bool HasEvent() const { return _hasEvent; }

    /// @brief Returns the current touch event.
    const TouchEvent& GetEvent() const { return _event; }

    /// @brief Returns whether the screen is currently being touched.
    bool IsTouching() const { return _state == State::Touching; }

    /// @brief Returns the current touch position.
    const Point& GetPosition() const { return _currentPos; }

private:
    static constexpr int VELOCITY_HISTORY_SIZE = 4;

    enum class State
    {
        Idle,
        Touching
    };

    struct SampleData
    {
        u16 x;
        u16 y;
        bool penDown;
    };

    State _state;
    Point _currentPos;
    Point _startPos;
    Point _prevPos;
    int _velocityX;
    int _velocityY;
    int _holdFrames;
    TouchEvent _event;
    bool _hasEvent;

    SampleData _sampleBuffer[4];
    u8 _sampleReadPtr;
    u8 _sampleWritePtr;

    struct VelocityEntry { int dx; int dy; };
    VelocityEntry _velocityHistory[VELOCITY_HISTORY_SIZE];
    int _velocityHistoryIdx;

    void ComputeVelocity();
};
