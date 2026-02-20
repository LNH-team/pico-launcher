#pragma once
#include "common.h"
#include <memory>
#include <libtwl/rtos/rtosMutex.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <../../libtwl7/include/libtwl/sound/soundChannel.h>
#include "fat/File.h"
#include "soundIpcCommand.h"
#include "IAudioStreamPlayer.h"

#define AUDIO_STREAM_PLAYER_RING_BLOCKS     32
#define AUDIO_STREAM_PLAYER_BLOCK_SAMPLES   256

/// @brief Class implementing an audio stream player.
class alignas(32) AudioStreamPlayer : public IAudioStreamPlayer
{
public:
    AudioStreamPlayer();

    ~AudioStreamPlayer()
    {
        StopPlayback();
    }

    bool StartPlayback(std::unique_ptr<IAudioStream> audioStream) override
    {
        bool result;
        rtos_lockMutex(&_mutex);
        {
            result = StartPlaybackIntern(std::move(audioStream));
        }
        rtos_unlockMutex(&_mutex);
        return result;
    }

    void StopPlayback() override
    {
        rtos_lockMutex(&_mutex);
        {
            StopPlaybackIntern();
        }
        rtos_unlockMutex(&_mutex);
    }

private:
    struct alignas(32) SoundStartCmdList
    {
        u32 cmdCount;
        snd_ipc_cmd_setup_channel_t leftSetup;
        snd_ipc_cmd_setup_channel_t rightSetup;
        u32 startChannels;
    };

    struct alignas(32) SoundStopCmdList
    {
        u32 cmdCount;
        u32 stopChannels;
    };

    s16 _audioRingL[AUDIO_STREAM_PLAYER_RING_BLOCKS][AUDIO_STREAM_PLAYER_BLOCK_SAMPLES] alignas(32);
    s16 _audioRingR[AUDIO_STREAM_PLAYER_RING_BLOCKS][AUDIO_STREAM_PLAYER_BLOCK_SAMPLES] alignas(32);
    SoundStartCmdList _soundStartCmdList alignas(32)
    {
        3,
        {
            SND_IPC_CMD_SETUP_CHANNEL,
            0,
            _audioRingL,
            0,
            0,
            sizeof(_audioRingL) >> 2,
            SOUNDCNT_VOLUME(127) | SOUNDCNT_MODE_LOOP | SOUNDCNT_FORMAT_PCM16
        },
        {
            SND_IPC_CMD_SETUP_CHANNEL,
            1,
            _audioRingR,
            0,
            0,
            sizeof(_audioRingR) >> 2,
            SOUNDCNT_VOLUME(127) | SOUNDCNT_PAN(127) | SOUNDCNT_MODE_LOOP | SOUNDCNT_FORMAT_PCM16
        },
        (0b11 << 8) | SND_IPC_CMD_START_CHANNELS
    };
    SoundStopCmdList _soundStopCmdList alignas(32)
    {
        1,
        (0b11 << 8) | SND_IPC_CMD_STOP_CHANNELS
    };
    u32 _threadStack[2048 / sizeof(u32)] alignas(32);
    rtos_mutex_t _mutex;
    rtos_thread_t _thread;
    rtos_event_t _event;
    volatile bool _isPlaying = false;
    volatile u8 _readBlock;
    volatile u8 _writeBlock;
    std::unique_ptr<IAudioStream> _audioStream;

    static AudioStreamPlayer* sCurrentPlayer;

    bool StartPlaybackIntern(std::unique_ptr<IAudioStream> audioStream);
    void StopPlaybackIntern();
    void ThreadMain();
    void FillRingBlock(u32 block);
};
