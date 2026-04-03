#pragma once
#include "ipc/IpcService.h"
#include "ipcChannels.h"

class BackLightIpcService : public IpcService
{
public:
    BackLightIpcService()
        : IpcService(IPC_CHANNEL_BACKLI) { }

    void OnMessageReceived(u32 data) override;
};


