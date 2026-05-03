#include "common.h"
#include <nds/system.h>
#include <libtwl/spi/spiPmic.h>
#include <libtwl/i2c/i2cMcu.h>
#include <backlightIpcCommand.h>
#include "BacklightIpcService.h"


static inline bool mcu_setLightLevel(u8 val)
{
    return mcu_writeReg(MCU_REG_BACKLIGHT,val);
}

static inline u8 mcu_readLightLevel()
{
    return mcu_readReg(MCU_REG_BACKLIGHT);
}

void BackLightIpcService::HandleMessage(u32 data)
{
    const bli_ipc_cmd cmd = {.as_u32 = data};

    switch (cmd.cmd_id)
    {
        case (BLI_IPC_CMD_SET_CURRENT_LEVEL):
            setBacklightLevel(cmd.backlightLevel);
        //fall through
        case (BLI_IPC_CMD_GET_CURRENT_LEVEL):
            u8 level = getBacklightLevel();
            const bli_ipc_cmd result = {
                .cmd_id = BLI_IPC_CMD_CURRENT_LEVEL_UPDATED,
                .backlightLevel = level
            };
            SendResponseMessage(result.as_u32);
            break;
    }
}
void BackLightIpcService::setBacklightLevel(const u8 level) const
{
    if (isDSiMode())
    {
        u8 val = level-1;
        if (val>=5) val = 4;
        mcu_setLightLevel(val);
    }
    else
    {
        rtos_lockMutex(&gPMIC_Mutex);
        pmic_setBacklightLevel(level);
        rtos_unlockMutex(&gPMIC_Mutex);
    }
}
u8 BackLightIpcService::getBacklightLevel() const
{
    if (isDSiMode())
    {
        rtos_lockMutex(&gMCU_Mutex);
        u8 retval = mcu_readLightLevel()+1;
        rtos_unlockMutex(&gMCU_Mutex);
        return retval;
    }
    else
    {
        rtos_lockMutex(&gPMIC_Mutex);
        u8 retval = pmic_readRegister(PMIC_REG_BACKLIGHT)&PMIC_BACKLIGHT_MASK;
        rtos_unlockMutex(&gPMIC_Mutex);
        return retval;
    }
}