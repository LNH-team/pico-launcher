#include "common.h"
#include <nds/system.h>
#include <libtwl/sound/sound.h>
#include <libtwl/sound/soundChannel.h>
#include <libtwl/sound/soundCapture.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/timer/timer.h>
#include <libtwl/sound/sound.h>
#include <libtwl/ipc/ipcSync.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/sys/sysPower.h>
#include <libtwl/sio/sioRtc.h>
#include <libtwl/sio/sio.h>
#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/mem/memSwap.h>
#include <libtwl/i2c/i2cMcu.h>
#include <libtwl/spi/spiPmic.h>
#include "logger/PlainLogger.h"
#include "logger/NocashOutputStream.h"
#include "logger/NullLogger.h"
#include "logger/ThreadSafeLogger.h"
#include "picoLoaderBootstrap.h"
#include "sharedMemory.h"
#include "ipcServices/DsiSdIpcService.h"
#include "ipcServices/DldiIpcService.h"
#include "ipcServices/SoundIpcService.h"
#include "ipcServices/RtcIpcService.h"
#include "ExitMode.h"
#include "Arm7State.h"
#include "mmc/tmio.h"
#include "ipcServices/BackLightIpcService.h"

static NocashOutputStream sNocashOutputStream;
static PlainLogger sPlainLogger = PlainLogger(LogLevel::All, std::unique_ptr<IOutputStream>(&sNocashOutputStream));
static ThreadSafeLogger sThreadSafeLogger = ThreadSafeLogger(std::unique_ptr<ILogger>(&sPlainLogger));

static DsiSdIpcService sDsiSdIpcService;
static DldiIpcService sDldiIpcService;
static SoundIpcService sSoundIpcService;
static RtcIpcService sRtcIpcService;
static BackLightIpcService sBackLightIpcService;

ILogger* gLogger = &sThreadSafeLogger;

static rtos_event_t sVBlankEvent;
static ExitMode sExitMode;
static Arm7State sState;
static volatile u8 sMcuIrqFlag = false;

static void vblankIrq(u32 irqMask)
{
    rtos_signalEvent(&sVBlankEvent);
}

static void vcountIrq(u32 irqMask)
{
    SHARED_KEY_XY = REG_RCNT0_H;
}

static void mcuIrq(u32 irq2Mask)
{
    sMcuIrqFlag = true;
}

static void checkMcuIrq(void)
{
    // mcu only exists in DSi mode
    if (isDSiMode())
    {
        // check and ack the flag atomically
        if (mem_swapByte(false, &sMcuIrqFlag))
        {
            // check the irq mask
            u32 irqMask = mcu_getIrqMask();
            if (irqMask & MCU_IRQ_RESET)
            {
                // power button was released
                sExitMode = ExitMode::Reset;
                sState = Arm7State::ExitRequested;
            }
            else if (irqMask & MCU_IRQ_POWER_OFF)
            {
                // power button was held long to trigger a power off
                sExitMode = ExitMode::PowerOff;
                sState = Arm7State::ExitRequested;
            }
        }
    }
}

static void initializeVBlankIrq()
{
    rtos_createEvent(&sVBlankEvent);
    rtos_setIrqFunc(RTOS_IRQ_VBLANK, vblankIrq);
    rtos_enableIrqMask(RTOS_IRQ_VBLANK);
    gfx_setVBlankIrqEnabled(true);
}

static void clearSoundRegisters()
{
    REG_SOUNDCNT = 0;
    REG_SNDCAP0CNT = 0;
    REG_SNDCAP1CNT = 0;

    for (int i = 0; i < 16; i++)
    {
        REG_SOUNDxCNT(i) = 0;
        REG_SOUNDxSAD(i) = 0;
        REG_SOUNDxTMR(i) = 0;
        REG_SOUNDxPNT(i) = 0;
        REG_SOUNDxLEN(i) = 0;
    }
}

static void initializeArm7()
{
    rtos_initIrq();
    rtos_startMainThread();
    ipc_initFifoSystem();

    clearSoundRegisters();

    pmic_setAmplifierEnable(true);
    sys_setSoundPower(true);

    readUserSettings();
    pmic_setPowerLedBlink(PMIC_CONTROL_POWER_LED_BLINK_NONE);

    sio_setGpioSiIrq(false);
    sio_setGpioMode(RCNT0_L_MODE_GPIO);

    rtc_init();

    if (isDSiMode())
    {
        TMIO_init();
        sDsiSdIpcService.Start();
    }

    sDldiIpcService.Start();
    pload_init();

    snd_setMasterVolume(127);
    snd_setMasterEnable(true);
    sSoundIpcService.Start();
    sRtcIpcService.Start();
    sBackLightIpcService.Start();

    gfx_setVCountMatchLine(96);
    rtos_setIrqFunc(RTOS_IRQ_VCOUNT, vcountIrq);
    rtos_enableIrqMask(RTOS_IRQ_VCOUNT);
    gfx_setVCountMatchIrqEnabled(true);

    initializeVBlankIrq();

    if (isDSiMode())
    {
        rtos_setIrq2Func(RTOS_IRQ2_MCU, mcuIrq);
        rtos_enableIrq2Mask(RTOS_IRQ2_MCU);
    }

    ipc_setArm7SyncBits(7);
}

static void updateArm7IdleState()
{
    if (pload_shouldStart())
    {
        sExitMode = ExitMode::PicoLoader;
        sState = Arm7State::ExitRequested;
    }
    else
    {
        checkMcuIrq();
    }

    if (sState == Arm7State::ExitRequested)
    {
        snd_setMasterVolume(0); // mute sound
    }
}

static bool performExit(ExitMode exitMode)
{
    switch (exitMode)
    {
        case ExitMode::Reset:
        {
            mcu_setWarmBootFlag(true);
            mcu_hardReset();
            break;
        }
        case ExitMode::PowerOff:
        {
            pmic_shutdown();
            break;
        }
        case ExitMode::PicoLoader:
        {
            pload_start();
            break;
        }
    }

    while (true); // wait infinitely for exit
}

static void updateArm7ExitRequestedState()
{
    performExit(sExitMode);
}

static void updateArm7()
{
    switch (sState)
    {
        case Arm7State::Idle:
        {
            updateArm7IdleState();
            break;
        }
        case Arm7State::ExitRequested:
        {
            updateArm7ExitRequestedState();
            break;
        }
    }
}

int main()
{
    sState = Arm7State::Idle;
    initializeArm7();

    while (true)
    {
        rtos_waitEvent(&sVBlankEvent, true, true);
        updateArm7();
    }

    return 0;
}
