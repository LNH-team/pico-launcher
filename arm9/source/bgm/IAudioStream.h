#pragma once

/// @brief Interface for an audio stream.
class IAudioStream
{
public:
    virtual ~IAudioStream() = 0;

    /// @brief Returns the sample rate of the audio stream in Hertz.
    /// @return The sample rate in Hertz.
    virtual u32 GetSampleRate() const = 0;

    /// @brief Reads the given number of samples from the audio stream
    ///        to the given buffers for the left and right channel.
    /// @param left The left channel buffer.
    /// @param right The right channel buffer.
    /// @param count The number of samples to read.
    virtual void ReadSamples(s16* left, s16* right, u32 count) = 0;

    /// @brief Closes the audio stream.
    virtual void Close() = 0;

    /// @brief Consumes a loop/restart notification, if supported by the stream.
    /// @return True if the stream looped/restarted since the last consume.
    virtual bool ConsumeLooped() { return false; }
};

inline IAudioStream::~IAudioStream() { }
