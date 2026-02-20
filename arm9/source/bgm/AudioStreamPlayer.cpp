#include "common.h"
#include <nds/arm9/cache.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/timer/timer.h>
#include <libtwl/rtos/rtosIrq.h>
#include "ipcChannels.h"
#include "AudioStreamPlayer.h"

/// @brief Hardware timer used for tracking audio blocks.
#define AUDIO_STREAM_PLAYER_TIMER           3

/// @brief Number of blocks that are not overwritten
///        between the read and write pointer to prevent
///        touching the current playing position.
#define AUDIO_STREAM_PLAYER_SAFETY_BLOCKS   4

/// @brief Priority of the stream thread.
#define AUDIO_STREAM_PLAYER_THREAD_PRIORITY 15

AudioStreamPlayer* AudioStreamPlayer::sCurrentPlayer = nullptr;

AudioStreamPlayer::AudioStreamPlayer()
{
    rtos_createMutex(&_mutex);
    rtos_createEvent(&_event);
    DC_FlushRange(&_soundStopCmdList, sizeof(_soundStopCmdList));
}

bool AudioStreamPlayer::StartPlaybackIntern(std::unique_ptr<IAudioStream> audioStream)
{
    if (_isPlaying)
        StopPlaybackIntern();

    _audioStream = std::move(audioStream);
    
    // fill buffer
    _readBlock = 0;
    _writeBlock = 0;
    for (u32 i = 0; i < AUDIO_STREAM_PLAYER_RING_BLOCKS - 1; i++)
    {
        FillRingBlock(i);
        _writeBlock++;
    }

    u32 rate = _audioStream->GetSampleRate();
    u32 timer = -((33513982 + rate) / (rate * 2));
    _soundStartCmdList.leftSetup.timer = timer;
    _soundStartCmdList.rightSetup.timer = timer;
    DC_FlushRange(&_soundStartCmdList, sizeof(_soundStartCmdList));

    sCurrentPlayer = this;

    rtos_disableIrqMask(RTOS_IRQ_TIMER(AUDIO_STREAM_PLAYER_TIMER));
    rtos_ackIrqMask(RTOS_IRQ_TIMER(AUDIO_STREAM_PLAYER_TIMER));
    rtos_setIrqFunc(RTOS_IRQ_TIMER(AUDIO_STREAM_PLAYER_TIMER), [] (u32 irqMask)
    {
        u32 readBlock = sCurrentPlayer->_readBlock;
        if (++readBlock == AUDIO_STREAM_PLAYER_RING_BLOCKS)
            readBlock = 0;
        sCurrentPlayer->_readBlock = readBlock;
        rtos_signalEvent(&sCurrentPlayer->_event);
    });
    tmr_configure(AUDIO_STREAM_PLAYER_TIMER, TMCNT_H_CLK_SYS_DIV_256, timer << 1, true);

    rtos_createThread(&_thread, AUDIO_STREAM_PLAYER_THREAD_PRIORITY, [] (void* arg)
    {
        static_cast<AudioStreamPlayer*>(arg)->ThreadMain();
    }, this, _threadStack, sizeof(_threadStack));

    tmr_start(AUDIO_STREAM_PLAYER_TIMER);
    rtos_enableIrqMask(RTOS_IRQ_TIMER(AUDIO_STREAM_PLAYER_TIMER));

    _isPlaying = true;
    rtos_wakeupThread(&_thread);
    ipc_sendFifoMessage(IPC_CHANNEL_SOUND, (u32)&_soundStartCmdList);

    return true;
}

void AudioStreamPlayer::StopPlaybackIntern()
{
    if (!_isPlaying)
        return;

    _isPlaying = false;
    rtos_wakeupThread(&_thread);
    ipc_sendFifoMessage(IPC_CHANNEL_SOUND, (u32)&_soundStopCmdList);
    tmr_stop(AUDIO_STREAM_PLAYER_TIMER);
    rtos_disableIrqMask(RTOS_IRQ_TIMER(AUDIO_STREAM_PLAYER_TIMER));
    rtos_joinThread(&_thread);
    sCurrentPlayer = nullptr;
    _audioStream.reset();
}

void AudioStreamPlayer::ThreadMain()
{
    do
    {
        bool doUpdate = true;
        while(_isPlaying && doUpdate)
        {
            rtos_lockMutex(&_mutex);
            {
                u32 writeBlock = _writeBlock;
                int freeBlocks = _readBlock - writeBlock - 1;
                if (freeBlocks < 0)
                    freeBlocks += AUDIO_STREAM_PLAYER_RING_BLOCKS;

                if (freeBlocks > AUDIO_STREAM_PLAYER_SAFETY_BLOCKS)
                {
                    FillRingBlock(writeBlock);
                    if (++writeBlock == AUDIO_STREAM_PLAYER_RING_BLOCKS)
                        writeBlock = 0;
                    _writeBlock = writeBlock;
                }
                else
                {
                    doUpdate = false;
                }
            }
            rtos_unlockMutex(&_mutex);
        }

        if (!_isPlaying)
            break;

        rtos_waitEvent(&_event, false, true);
    } while(_isPlaying);
}

void AudioStreamPlayer::FillRingBlock(u32 block)
{
    s16* blockPtrL = &_audioRingL[block][0];
    s16* blockPtrR = &_audioRingR[block][0];

    _audioStream->ReadSamples(blockPtrL, blockPtrR, AUDIO_STREAM_PLAYER_BLOCK_SAMPLES);

    DC_FlushRange(&blockPtrL, AUDIO_STREAM_PLAYER_BLOCK_SAMPLES * sizeof(s16));
    DC_FlushRange(&blockPtrR, AUDIO_STREAM_PLAYER_BLOCK_SAMPLES * sizeof(s16));
}
