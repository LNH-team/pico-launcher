#include <nds/ndstypes.h>
#include <nds/system.h>
#include <libtwl/spi/spiPmic.h>
#include <libtwl/i2c/i2cMcu.h>
// #include "core/mini-printf.h"
#include "BackLightIpcService.h"

// #define NOCASHDEBUG *(vu32*)(0x04FFFA10)

static inline bool mcu_setLightLevel(u8 val)
{
    return mcu_writeReg(MCU_REG_BACKLIGHT,val);
}

void BackLightIpcService::OnMessageReceived(u32 data)
{
    // static char buff[256];
    // mini_snprintf(buff,sizeof(buff),"NCSH: data:%x\n\n",data);
    // NOCASHDEBUG = (u32)buff;
    // NOCASHDEBUG = (u32)("NCSH: BackLightStart.\n\n");
    if (isDSiMode())
    {
        // NOCASHDEBUG = (u32)("NCSH: Here_dsi.1\n\n");
        u8 val = ((data)&0b111)-1;
        // NOCASHDEBUG = (u32)("NCSH: Here_dsi.2\n\n");
        // asm volatile ("mov r11,r11":::"memory");
        mcu_setLightLevel(val);
        // if (allgood)
        //     // NOCASHDEBUG = (u32)("NCSH: All_good\n\n");
        // else
        //     NOCASHDEBUG = (u32)("NCSH: Not_good\n\n");
        // NOCASHDEBUG = (u32)("NCSH: Here_dsi.3\n\n");
        return;
    }
    pmic_setBacklightLevel(data);
    // NOCASHDEBUG = (u32)("NCSH: Here 10.\n\n");
}
