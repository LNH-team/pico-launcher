#pragma once
#include "ipc/ThreadIpcService.h"
#include "ipcChannels.h"

class StatusIpcService : public ThreadIpcService
{
public:
    StatusIpcService()
        : ThreadIpcService(IPC_CHANNEL_STATUS, 10, _threadStack, sizeof(_threadStack)) { }

    /// @brief Fills a StatusPayload for ARM9 and signals completion.
    void HandleMessage(u32 data) override;

private:
    u32 _threadStack[256];
};
