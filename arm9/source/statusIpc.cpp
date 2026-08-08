#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/rtos/rtosMutex.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include "ipcChannels.h"
#include "statusIpc.h"

static rtos_mutex_t sMutex;
static rtos_event_t sEvent;

alignas(32) static union {
    StatusPayload payload;
    u8 sizeAlign[32];   ///< Pads the transfer buffer to a full cache line.
} sStatus;

static void ipcMessageHandler(u32 channel, u32 data, void* arg) { rtos_signalEvent(&sEvent); }

void status_init()
{
    rtos_createMutex(&sMutex);
    rtos_createEvent(&sEvent);
    ipc_setChannelHandler(IPC_CHANNEL_STATUS, ipcMessageHandler, nullptr);
}

void status_read(StatusPayload* out)
{
    rtos_lockMutex(&sMutex);
    {
        rtos_clearEvent(&sEvent);
        DC_InvalidateRange(&sStatus, sizeof(sStatus));
        ipc_sendFifoMessage(IPC_CHANNEL_STATUS, (u32)&sStatus >> 2);
        rtos_waitEvent(&sEvent, false, true);
        memcpy(out, &sStatus.payload, sizeof(StatusPayload));
    }
    rtos_unlockMutex(&sMutex);
}
