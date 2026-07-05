#pragma once
#include "ipc/ThreadIpcService.h"
#include "ipcChannels.h"
#include "ThreadIpcServicePriorities.h"

class BackLightIpcService : public ThreadIpcService
{
    u32 _threadStack[128];
    u8 getBacklightLevel()const;
    void setBacklightLevel(const u8 level)const;

public:
    BackLightIpcService()
        : ThreadIpcService(IPC_CHANNEL_BACKLIGHT, IPC_PRIORITY_BACKLIGHT, _threadStack, sizeof(_threadStack)) { }

    void HandleMessage(u32 data) override;
};

