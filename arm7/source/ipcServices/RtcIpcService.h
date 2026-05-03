#pragma once
#include "ipc/ThreadIpcService.h"
#include "ipcChannels.h"
#include "ThreadIpcServicePriorities.h"

class RtcIpcService : public ThreadIpcService
{
    u32 _threadStack[128];

public:
    RtcIpcService()
        : ThreadIpcService(IPC_CHANNEL_RTC, IPC_PRIORITY_RTC, _threadStack, sizeof(_threadStack)) { }

    void HandleMessage(u32 data) override;
};
