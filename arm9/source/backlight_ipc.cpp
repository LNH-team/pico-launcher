#include "common.h"
#include <backlightIpcCommand.h>
#include <ipcChannels.h>

static u8 sBacklightLevel;
static rtos_event_t sEvent;

static void ipcMessageHandler(u32 channel, u32 data, void* arg)
{
    const bli_ipc_cmd cmd={.as_u32 = data};
    if (cmd.cmd_id==BLI_IPC_CMD_CURRENT_LEVEL_UPDATED)
        sBacklightLevel = cmd.backlightLevel;
    rtos_signalEvent(&sEvent);
}

void initBacklightIpc()
{
    ipc_setChannelHandler(IPC_CHANNEL_BACKLIGHT,ipcMessageHandler,nullptr);

    const u32 CMD_FETCH_LEVEL = bli_ipc_cmd{
        .cmd_id=BLI_IPC_CMD_GET_CURRENT_LEVEL
    }.as_u32;

    ipc_sendFifoMessage(IPC_CHANNEL_BACKLIGHT,CMD_FETCH_LEVEL);
    rtos_waitEvent(&sEvent, false, true);
}

extern "C" u8 setBacklightlevel(u8 level)
{
    const u32 CMD_SET = bli_ipc_cmd{
        .cmd_id=BLI_IPC_CMD_SET_CURRENT_LEVEL,
        .backlightLevel = level
    }.as_u32;

    ipc_sendFifoMessage(IPC_CHANNEL_BACKLIGHT, CMD_SET);
    rtos_waitEvent(&sEvent, false, true);

    return sBacklightLevel;
}

extern "C" u8 getBacklightlevel()
{
    return sBacklightLevel;
}
