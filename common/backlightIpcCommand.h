#include <nds/ndstypes.h>
#include "libtwl/ipc/ipcFifoSystem.h"

enum backlightIpcCommand{
    BLI_IPC_CMD_GET_CURRENT_LEVEL,
    BLI_IPC_CMD_SET_CURRENT_LEVEL,
    BLI_IPC_CMD_CURRENT_LEVEL_UPDATED,
};

union bli_ipc_cmd{
    struct {
        u8 cmd_id;
        u8 backlightLevel;
        //note last 5 bits are reserved for IPC_FIFO_MSG_CHANNEL_BITS
    };
    u32 as_u32;
};
