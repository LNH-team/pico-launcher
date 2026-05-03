#pragma once
#include "ipc/ThreadIpcService.h"
#include "dldiIpcCommand.h"
#include "ipcChannels.h"
#include "ThreadIpcServicePriorities.h"

class DldiIpcService : public ThreadIpcService
{
    u32 _threadStack[128];

    void SetupDldi(const dldi_ipc_cmd_t* cmd) const;
    void ReadSectors(const dldi_ipc_cmd_t* cmd) const;
    void WriteSectors(const dldi_ipc_cmd_t* cmd) const;

public:
    DldiIpcService()
        : ThreadIpcService(IPC_CHANNEL_DLDI, IPC_PRIORITY_DLDI, _threadStack, sizeof(_threadStack)) { }

    void HandleMessage(u32 data) override;
};
