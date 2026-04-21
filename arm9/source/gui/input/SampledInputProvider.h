#pragma once
#include "InputProvider.h"
#include "IKeyInputSource.h"
#include "ITouchInputSource.h"

/// @brief Input provider providing input from an \see IKeyInputSource.
class SampledInputProvider : public InputProvider
{
public:
    explicit SampledInputProvider(const IKeyInputSource* keyInputSource, const ITouchInputSource* touchInputSource)
        : _keyInputSource(keyInputSource), _touchInputSource(touchInputSource)
        , _inputBufferReadPtr(0), _inputBufferWritePtr(0) { }

    void Update() override;

    /// @brief Samples the input source.
    void Sample()
    {
        _keyInputBuffer[_inputBufferWritePtr] = _keyInputSource->Sample();
        _touchInputSource->Sample(_touchInputBuffer[_inputBufferWritePtr]);
        _inputBufferWritePtr = (_inputBufferWritePtr + 1) & 3;
    }

    void Reset() override
    {
        InputProvider::Reset();
        _inputBufferReadPtr = 0;
        _inputBufferWritePtr = 0;
    }

private:
    const IKeyInputSource* _keyInputSource;
    const ITouchInputSource* _touchInputSource;

    InputKey _keyInputBuffer[4];
    Point _touchInputBuffer[4];
    u8 _inputBufferReadPtr;
    u8 _inputBufferWritePtr;
};
