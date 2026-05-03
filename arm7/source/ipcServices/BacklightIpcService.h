#pragma once
#include "ipc/ThreadIpcService.h"
#include "ipcChannels.h"
class BackLightIpcService : public ThreadIpcService
{
    u32 _threadStack[128];
    u8 getBacklightLevel()const;
    void setBacklightLevel(const u8 level)const;

public:
    BackLightIpcService()//No clue if this is a good priority / thread stack size
        : ThreadIpcService(IPC_CHANNEL_BACKLIGHT, 4 , _threadStack, sizeof(_threadStack)) { }

    void HandleMessage(u32 data) override;
};

