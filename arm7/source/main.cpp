#include "common.h"
#include <nds/system.h>
#include <libtwl/sound/sound.h>
#include <libtwl/sound/soundChannel.h>
#include <libtwl/sound/soundCapture.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/timer/timer.h>
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
#include <libtwl/spi/spi.h>
#include <libtwl/spi/spiCodec.h>
#include "sharedMemory.h"
#include "ipcServices/DsiSdIpcService.h"
#include "ipcServices/DldiIpcService.h"
#include "ipcServices/SoundIpcService.h"
#include "ipcServices/RtcIpcService.h"
#include "ExitMode.h"
#include "Arm7State.h"
#include "mmc/tmio.h"

static NocashOutputStream sNocashOutputStream;
static PlainLogger sPlainLogger = PlainLogger(LogLevel::All, std::unique_ptr<IOutputStream>(&sNocashOutputStream));
static ThreadSafeLogger sThreadSafeLogger = ThreadSafeLogger(std::unique_ptr<ILogger>(&sPlainLogger));

static DsiSdIpcService sDsiSdIpcService;
static DldiIpcService sDldiIpcService;
static SoundIpcService sSoundIpcService;
static RtcIpcService sRtcIpcService;

ILogger* gLogger = &sThreadSafeLogger;

static rtos_event_t sVBlankEvent;
static ExitMode sExitMode;
static Arm7State sState;
static volatile u8 sMcuIrqFlag = false;

static void vblankIrq(u32 irqMask)
{
    rtos_signalEvent(&sVBlankEvent);
}

static u16 touchSpiReadAxis(u8 command)
{
    spi_transferByte(SPICNT_DEVICE_TOUCH | SPICNT_SPEED_2_MHZ, command);
    u8 hi = spi_transferByte(SPICNT_DEVICE_TOUCH | SPICNT_SPEED_2_MHZ, 0);
    u8 lo = spi_transferLastByte(SPICNT_DEVICE_TOUCH | SPICNT_SPEED_2_MHZ, 0);
    return ((hi & 0x7F) << 5) | ((lo >> 3) & 0x1F);
}

struct TouchCalibration
{
    s32 xScale; 
    s32 yScale;
    s32 xOffset;
    s32 yOffset;
    bool initialized;
};

static TouchCalibration sTouchCalibration = { 0, 0, 0, 0, false };

static void touchInitCalibration()
{
    const volatile u8* personal = (const volatile u8*)0x027FFC80;
    u16 calX1 = *(vu16*)(personal + 0x58);
    u16 calY1 = *(vu16*)(personal + 0x5A);
    u8  calX1px = *(vu8*)(personal + 0x5C);
    u8  calY1px = *(vu8*)(personal + 0x5D);
    u16 calX2 = *(vu16*)(personal + 0x5E);
    u16 calY2 = *(vu16*)(personal + 0x60);
    u8  calX2px = *(vu8*)(personal + 0x62);
    u8  calY2px = *(vu8*)(personal + 0x63);

    if (calX2 != calX1 && calY2 != calY1)
    {
        sTouchCalibration.xScale = ((s32)(calX2px - calX1px) << 12) / (s32)(calX2 - calX1);
        sTouchCalibration.yScale = ((s32)(calY2px - calY1px) << 12) / (s32)(calY2 - calY1);
        sTouchCalibration.xOffset = ((s32)calX1px << 12) - sTouchCalibration.xScale * (s32)calX1;
        sTouchCalibration.yOffset = ((s32)calY1px << 12) - sTouchCalibration.yScale * (s32)calY1;
    }
    else
    {
        sTouchCalibration.xScale = (256 << 12) / 4096;
        sTouchCalibration.yScale = (192 << 12) / 4096;
        sTouchCalibration.xOffset = 0;
        sTouchCalibration.yOffset = 0;
    }
    sTouchCalibration.initialized = true;
}

static s32 clampS32(s32 val, s32 minVal, s32 maxVal)
{
    if (val < minVal) return minVal;
    if (val > maxVal) return maxVal;
    return val;
}

static u32 touchAbs(s32 x)
{
    return x >= 0 ? (u32)x : (u32)(-x);
}

static u16 sNtrLatchX = 0;
static u16 sNtrLatchY = 0;
static bool sNtrHasLatch = false;

static void touchReadNtr()
{
    u32 xSum = 0, ySum = 0;
    for (int i = 0; i < 4; i++)
    {
        xSum += touchSpiReadAxis(0xD1);
        ySum += touchSpiReadAxis(0x91);
    }
    u16 rawX = (u16)(xSum >> 2);
    u16 rawY = (u16)(ySum >> 2);

    static const u32 DIFF_THRESHOLD = 20;
    bool valid = !sNtrHasLatch ||
        (touchAbs((s32)rawX - (s32)sNtrLatchX) < DIFF_THRESHOLD &&
         touchAbs((s32)rawY - (s32)sNtrLatchY) < DIFF_THRESHOLD);

    if (valid)
    {
        sNtrLatchX = rawX;
        sNtrLatchY = rawY;
        sNtrHasLatch = true;
    }

    s32 px = (sTouchCalibration.xScale * (s32)sNtrLatchX + sTouchCalibration.xOffset) >> 12;
    s32 py = (sTouchCalibration.yScale * (s32)sNtrLatchY + sTouchCalibration.yOffset) >> 12;
    SHARED_TOUCH_X = (u16)clampS32(px, 0, 255);
    SHARED_TOUCH_Y = (u16)clampS32(py, 0, 191);
}

#define CDC_TSC_REG_SAR_ADC_CTRL       0x02
#define CDC_TSC_REG_SAR_ADC_CONV_MODE  0x03
#define CDC_TSC_REG_PRECHARGE_SENSE    0x04
#define CDC_TSC_REG_PANEL_VOLT_STBLZ   0x05
#define CDC_TSC_REG_STATUS0            0x09
#define CDC_TSC_REG_BUFFER_MODE        0x0E
#define CDC_TSC_REG_SCAN_MODE_TIMER    0x0F
#define CDC_TSC_REG_DEBOUNCE_TIMER     0x12

#define CDC_PAGE_TSC_CONTROL  CODEC_PAGE_3
#define CDC_PAGE_TSC_DATA     0xFC

static bool sDsiTscInitialized = false;

static u16 sDsiLatchX = 0;
static u16 sDsiLatchY = 0;
static bool sDsiHasLatch = false;

static void codec_writeRegisterMask(u8 reg, u8 mask, u8 value)
{
    u8 old = codec_readRegister(reg);
    codec_writeRegister(reg, (old & ~mask) | (value & mask));
}

static void codec_readRegisterArray(u8 startReg, u8* data, u32 len)
{
    if (len == 0) return;

    spi_transferByte(SPICNT_DEVICE_TOUCH | SPICNT_SPEED_2_MHZ, (startReg << 1) | 1);

    for (u32 i = 0; i < len - 1; i++)
    {
        data[i] = spi_transferByte(SPICNT_DEVICE_TOUCH | SPICNT_SPEED_2_MHZ, 0);
    }

    data[len - 1] = spi_transferLastByte(SPICNT_DEVICE_TOUCH | SPICNT_SPEED_2_MHZ, 0);
}

static void touchDsiInitTsc()
{
    codec_setPage(CDC_PAGE_TSC_CONTROL);

    codec_writeRegisterMask(CDC_TSC_REG_BUFFER_MODE, 0x80, 0);

    codec_writeRegisterMask(CDC_TSC_REG_SAR_ADC_CTRL, 0x18, 3 << 3);

    codec_writeRegister(CDC_TSC_REG_SCAN_MODE_TIMER, 0xA0);

    codec_writeRegisterMask(CDC_TSC_REG_BUFFER_MODE, 0x38, 5 << 3);

    codec_writeRegisterMask(CDC_TSC_REG_BUFFER_MODE, 0x40, 0);

    codec_writeRegister(CDC_TSC_REG_SAR_ADC_CONV_MODE, 0x87);

    codec_writeRegisterMask(CDC_TSC_REG_PANEL_VOLT_STBLZ, 0x07, 4);

    codec_writeRegisterMask(CDC_TSC_REG_PRECHARGE_SENSE, 0x07, 6);

    codec_writeRegisterMask(CDC_TSC_REG_PRECHARGE_SENSE, 0x70, 4 << 4);

    codec_writeRegisterMask(CDC_TSC_REG_DEBOUNCE_TIMER, 0x07, 0);

    codec_writeRegisterMask(CDC_TSC_REG_BUFFER_MODE, 0x80, 0x80);

    sDsiTscInitialized = true;
}

static bool touchReadDsi()
{
    if (!sDsiTscInitialized)
        touchDsiInitTsc();

    codec_setPage(CDC_PAGE_TSC_CONTROL);

    u8 status = codec_readRegister(CDC_TSC_REG_STATUS0);
    if ((status & 0xC0) == 0x40)
    {
        sDsiHasLatch = false;
        return false;
    }

    u8 bufMode = codec_readRegister(CDC_TSC_REG_BUFFER_MODE);
    if (bufMode & 0x02)
    {
        sDsiHasLatch = false;
        return false;
    }

    codec_setPage(CDC_PAGE_TSC_DATA);
    u8 raw[20];
    codec_readRegisterArray(0x01, raw, 20);

    u16 arrayX[5], arrayY[5];
    for (int i = 0; i < 5; i++)
    {
        arrayX[i] = ((u16)raw[i * 2 + 0] << 8) | raw[i * 2 + 1];
        arrayY[i] = ((u16)raw[i * 2 + 10] << 8) | raw[i * 2 + 11];

        if ((arrayX[i] & 0xF000) || (arrayY[i] & 0xF000))
        {
            sDsiHasLatch = false;
            return false;
        }
    }

    static const u32 DIFF_THRESHOLD = 20;
    u16 finalX, finalY;
    bool valid = false;

    for (int i = 0; !valid && i < 4; i++)
    {
        u32 sumX = arrayX[i];
        u32 sumY = arrayY[i];
        u32 numValid = 1;

        for (int j = 0; j < 5; j++)
        {
            if (i == j) continue;
            u32 diffX = touchAbs((s32)arrayX[i] - (s32)arrayX[j]);
            u32 diffY = touchAbs((s32)arrayY[i] - (s32)arrayY[j]);
            if (diffX < DIFF_THRESHOLD && diffY < DIFF_THRESHOLD)
            {
                sumX += arrayX[j];
                sumY += arrayY[j];
                numValid++;
            }
        }

        if (numValid >= 3)
        {
            finalX = (u16)(sumX / numValid);
            finalY = (u16)(sumY / numValid);
            valid = true;
        }
    }

    if (valid)
    {
        sDsiLatchX = finalX;
        sDsiLatchY = finalY;
        sDsiHasLatch = true;
    }
    else
    {
        if (!sDsiHasLatch)
            return false;
        finalX = sDsiLatchX;
        finalY = sDsiLatchY;
    }

    s32 px = (sTouchCalibration.xScale * (s32)finalX + sTouchCalibration.xOffset) >> 12;
    s32 py = (sTouchCalibration.yScale * (s32)finalY + sTouchCalibration.yOffset) >> 12;
    SHARED_TOUCH_X = (u16)clampS32(px, 0, 255);
    SHARED_TOUCH_Y = (u16)clampS32(py, 0, 191);
    return true;
}


static void vcountIrq(u32 irqMask)
{
    SHARED_KEY_XY = REG_RCNT0_H;

    if (isDSiMode())
    {
        if (!sTouchCalibration.initialized)
            touchInitCalibration();

        if (touchReadDsi())
            SHARED_KEY_XY &= ~(1 << 6);
        else
            SHARED_KEY_XY |= (1 << 6);
        return;
    }

    if (!(REG_RCNT0_H & (1 << 6)))
    {
        if (!sTouchCalibration.initialized)
            touchInitCalibration();

        touchReadNtr();
    }
    else
    {
        sNtrHasLatch = false;
    }
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
