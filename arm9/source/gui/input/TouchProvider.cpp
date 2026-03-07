#include "common.h"
#include "TouchProvider.h"

TouchProvider::TouchProvider()
    : _state(State::Idle)
    , _currentPos(0, 0), _startPos(0, 0), _prevPos(0, 0)
    , _velocityX(0), _velocityY(0), _holdFrames(0)
    , _hasEvent(false)
    , _sampleReadPtr(0), _sampleWritePtr(0)
    , _velocityHistoryIdx(0)
{
    for (int i = 0; i < VELOCITY_HISTORY_SIZE; i++)
    {
        _velocityHistory[i] = { 0, 0 };
    }
}

void TouchProvider::Sample()
{
    if (SHARED_SYSTEM_FLAGS & SHARED_FLAG_SLEEP_MODE)
    {
        auto& sample = _sampleBuffer[_sampleWritePtr];
        sample.penDown = false;
        sample.x = 0;
        sample.y = 0;
        _sampleWritePtr = (_sampleWritePtr + 1) & 3;
        return;
    }

    bool penDown = !(SHARED_KEY_XY & (1 << 6));

    auto& sample = _sampleBuffer[_sampleWritePtr];
    sample.penDown = penDown;
    if (penDown)
    {
        sample.x = SHARED_TOUCH_X;
        sample.y = SHARED_TOUCH_Y;
    }
    else
    {
        sample.x = 0;
        sample.y = 0;
    }
    _sampleWritePtr = (_sampleWritePtr + 1) & 3;
}

void TouchProvider::Update()
{
    _hasEvent = false;

    bool penDown = (_state == State::Touching);
    int sumX = 0, sumY = 0, penDownCount = 0;

    while (_sampleReadPtr != _sampleWritePtr)
    {
        const auto& sample = _sampleBuffer[_sampleReadPtr];
        penDown = sample.penDown;
        if (penDown)
        {
            sumX += sample.x;
            sumY += sample.y;
            penDownCount++;
        }
        _sampleReadPtr = (_sampleReadPtr + 1) & 3;
    }

    u16 lastX = _currentPos.x;
    u16 lastY = _currentPos.y;
    if (penDown && penDownCount > 0)
    {
        lastX = (u16)(sumX / penDownCount);
        lastY = (u16)(sumY / penDownCount);
    }

    static constexpr int NOISE_REJECT_THRESHOLD = 30;
    if (penDown && penDownCount > 0 && _state == State::Touching)
    {
        int dx = (int)lastX - (int)_currentPos.x;
        int dy = (int)lastY - (int)_currentPos.y;
        if (dx * dx + dy * dy > NOISE_REJECT_THRESHOLD * NOISE_REJECT_THRESHOLD)
        {
            lastX = _currentPos.x;
            lastY = _currentPos.y;
        }
    }

    switch (_state)
    {
        case State::Idle:
        {
            if (penDown)
            {
                // Touch started
                _state = State::Touching;
                _currentPos = Point(lastX, lastY);
                _startPos = _currentPos;
                _prevPos = _currentPos;
                _holdFrames = 0;
                _velocityX = 0;
                _velocityY = 0;
                for (int i = 0; i < VELOCITY_HISTORY_SIZE; i++)
                    _velocityHistory[i] = { 0, 0 };
                _velocityHistoryIdx = 0;

                _event.type = TouchEventType::Down;
                _event.position = _currentPos;
                _event.startPosition = _startPos;
                _event.deltaX = 0;
                _event.deltaY = 0;
                _event.velocityX = 0;
                _event.velocityY = 0;
                _event.totalDeltaX = 0;
                _event.totalDeltaY = 0;
                _event.holdFrames = 0;
                _hasEvent = true;
            }
            break;
        }
        case State::Touching:
        {
            if (!penDown)
            {
                // Touch ended
                _state = State::Idle;
                ComputeVelocity();

                _event.type = TouchEventType::Up;
                _event.position = _currentPos;
                _event.startPosition = _startPos;
                _event.deltaX = 0;
                _event.deltaY = 0;
                _event.velocityX = _velocityX;
                _event.velocityY = _velocityY;
                _event.totalDeltaX = _currentPos.x - _startPos.x;
                _event.totalDeltaY = _currentPos.y - _startPos.y;
                _event.holdFrames = _holdFrames;
                _hasEvent = true;
            }
            else
            {
                // Touch ongoing
                _prevPos = _currentPos;
                _currentPos = Point(lastX, lastY);
                _holdFrames++;

                int dx = _currentPos.x - _prevPos.x;
                int dy = _currentPos.y - _prevPos.y;

                // Record delta for velocity smoothing
                _velocityHistory[_velocityHistoryIdx] = { dx, dy };
                _velocityHistoryIdx = (_velocityHistoryIdx + 1) % VELOCITY_HISTORY_SIZE;
                ComputeVelocity();

                _event.type = TouchEventType::Move;
                _event.position = _currentPos;
                _event.startPosition = _startPos;
                _event.deltaX = dx;
                _event.deltaY = dy;
                _event.velocityX = _velocityX;
                _event.velocityY = _velocityY;
                _event.totalDeltaX = _currentPos.x - _startPos.x;
                _event.totalDeltaY = _currentPos.y - _startPos.y;
                _event.holdFrames = _holdFrames;
                _hasEvent = true;
            }
            break;
        }
    }
}

void TouchProvider::ComputeVelocity()
{
    // Weighted average of recent deltas (more recent = higher weight)
    // Weights: 1, 2, 3, 4 (newest = 4)
    int totalDx = 0;
    int totalDy = 0;
    int totalWeight = 0;
    for (int i = 0; i < VELOCITY_HISTORY_SIZE; i++)
    {
        int idx = (_velocityHistoryIdx + i) % VELOCITY_HISTORY_SIZE;
        int weight = i + 1;
        totalDx += _velocityHistory[idx].dx * weight;
        totalDy += _velocityHistory[idx].dy * weight;
        totalWeight += weight;
    }
    // Velocity in fixed point 4.4 (pixels per frame * 16)
    _velocityX = (totalDx << 4) / totalWeight;
    _velocityY = (totalDy << 4) / totalWeight;
}

void TouchProvider::Reset()
{
    _state = State::Idle;
    _hasEvent = false;
    _sampleReadPtr = 0;
    _sampleWritePtr = 0;
    _holdFrames = 0;
    _velocityX = 0;
    _velocityY = 0;
}
