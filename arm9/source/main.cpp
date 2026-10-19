#include "common.h"
#include "core/di.h"
#include <libtwl/gfx/gfx.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/ipc/ipcSync.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include "services/process/ProcessManager.h"
#include "logger/PlainLogger.h"
#include "logger/NocashOutputStream.h"
#include "logger/SemihostingOutputStream.h"
#include "logger/NitroEmulatorOutputStream.h"
#include "logger/PicoAgbAdapterOutputStream.h"
#include "logger/NullLogger.h"
#include "logger/ThreadSafeLogger.h"
#include "dsiSdIpc.h"
#include "dldiIpc.h"
#include "fat/ff.h"
#include "services/settings/JsonAppSettingsService.h"
#include "App.h"
#include "core/Environment.h"
#include "rng/RandomGenerator.h"
#include "rng/LinearCongruentialGenerator.h"
#include "rng/ThreadSafeRandomGenerator.h"
#include "NotoSansJP-Regular-10_nft2.h"
#include "NotoSansJP-Medium-7_5_nft2.h"
#include "NotoSansJP-Medium-10_nft2.h"
#include "NotoSansJP-Medium-10-clock_nft2.h"
#include "NotoSansJP-Medium-11_nft2.h"
#include "gui/font/nitroFont2.h"
#include "picoLoaderBootstrap.h"
#include "rtcIpc.h"
#include "statusIpc.h"

ProcessManager gProcessManager;
ILogger* gLogger;
FATFS gFatFs;
RandomGenerator* gRandomGenerator;

static void initLogger()
{
    std::unique_ptr<IOutputStream> outputStream;
    if (Environment::IsIsNitroEmulator() && Environment::SupportsAgbSemihosting())
    {
        outputStream = std::make_unique<NitroEmulatorOutputStream>();
    }
    else if (Environment::SupportsJtagSemihosting())
    { 
        outputStream = std::make_unique<SemihostingOutputStream>();
    }
    else if (Environment::HasPicoAgbAdapter())
    {
        outputStream = std::make_unique<PicoAgbAdapterOutputStream>();
    }
    else if (Environment::SupportsNocashPrint())
    { 
        outputStream = std::make_unique<NocashOutputStream>();
    }
    else
    {
        gLogger = new NullLogger();
        return;
    }
    gLogger = new ThreadSafeLogger(std::make_unique<PlainLogger>(LogLevel::All, std::move(outputStream)));
}

static void initRandomGenerator()
{
    u64 seed = gTickCounter.GetValue() + 1430287ULL;
    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    seed = seed * 7302013ULL + dateTime.date.year;
    seed = seed * 7302013ULL + dateTime.date.month;
    seed = seed * 7302013ULL + dateTime.date.monthDay;
    seed = seed * 7302013ULL + dateTime.time.hour;
    seed = seed * 7302013ULL + dateTime.time.minute;
    seed = seed * 7302013ULL + dateTime.time.second;
    gRandomGenerator = new ThreadSafeRandomGenerator<LinearCongruentialGenerator>(seed);
}

static bool mountDldi()
{
    FRESULT res = f_mount(&gFatFs, "fat:", 1);
    if (res != FR_OK)
    {
        LOG_ERROR("dldi mount failed: %d\n", res);
        return false;
    }
    f_chdrive("fat:");
    return true;
}

static bool mountDsiSd()
{
    FRESULT res = f_mount(&gFatFs, "sd:", 1);
    if (res != FR_OK)
    {
        LOG_ERROR("dsi sd mount failed: %d\n", res);
        return false;
    }
    f_chdrive("sd:");
    return true;
}

static bool mountSemihosting()
{
    FRESULT res = f_mount(&gFatFs, "pc:", 1);
    if (res != FR_OK)
    {
        LOG_ERROR("pc sd mount failed: %d\n", res);
        return false;
    }
    f_chdrive("pc:");
    return true;
}

static bool mountAgbSemihosting()
{
    FRESULT res = f_mount(&gFatFs, "pc2:", 1);
    if (res != FR_OK)
    {
        LOG_ERROR("pc2 sd mount failed: %d\n", res);
        return false;
    }
    f_chdrive("pc2:");
    return true;
}

static bool shouldMountDsiSd(int argc, char* argv[], bool dldiInitSuccessfull)
{
    if (!Environment::IsDsiMode())
        return false;

    if (argc == 0)
        return !dldiInitSuccessfull;

    if (argv[0][0] == 'f' && argv[0][1] == 'a' && argv[0][2] == 't' && argv[0][3] == ':')
        return false;

    return true;
}

int main(int argc, char* argv[])
{
    REG_MASTER_BRIGHT = 0x4010;
    REG_MASTER_BRIGHT_SUB = 0x4010;

    bool dldiInitSuccessful = false;
    Environment::Initialize();

    heap_init();

    rtos_initIrq();
    rtos_startMainThread();

    // Insert bkpt #0 instruction in prefetch abort and data abort vectors for debugging purposes
    *(vu32*)0x0100000C = 0xE1200070;
    *(vu32*)0x01000010 = 0xE1200070;

    ipc_initFifoSystem();
    if (Environment::IsDsiMode())
        dsisd_init();

    VBlank::Init();
    initLogger();
    gTickCounter.Start();

    while (ipc_getArm7SyncBits() != 7);

    rtc_init();
    status_init();

    if (argc >= 1)
    {
        pload_setLauncherPath(argv[0]);
    }

    memset(&gFatFs, 0, sizeof(gFatFs));
    if (dldi_init())
    {
        mountDldi();
        pload_setBootDrive(PLOAD_BOOT_DRIVE_DLDI);
        dldiInitSuccessful = true;
    }

    LOG_DEBUG("ARM9 Start\n");
    if (Environment::IsDsiMode())
        LOG_DEBUG("Running in DSi mode\n");
    if (Environment::IsIsNitroEmulator())
        LOG_DEBUG("Running on IS-NITRO-EMULATOR\n");
    if (Environment::SupportsJtagSemihosting())
        LOG_DEBUG("JTAG semihosting supported\n");
    if (Environment::SupportsAgbSemihosting())
        LOG_DEBUG("AGB semihosting supported\n");
    if (Environment::SupportsNocashPrint())
        LOG_DEBUG("Nocash print supported\n");
    if (Environment::HasPicoAgbAdapter())
        LOG_DEBUG("Pico Agb Adapter connected\n");

    if (shouldMountDsiSd(argc, argv, dldiInitSuccessful))
    {
        mountDsiSd();
        pload_setBootDrive(PLOAD_BOOT_DRIVE_DSI_SD);
    }
    else if (Environment::SupportsAgbSemihosting())
    {
        mountAgbSemihosting();
        pload_setBootDrive(PLOAD_BOOT_DRIVE_AGB_SEMIHOSTING);
    }
    else if (Environment::SupportsJtagSemihosting())
    {
        mountSemihosting();
    }

    initRandomGenerator();

    // todo: make sure _pico folder exists
    // maybe warn if important files are missing as well?

    nft2_unpack((nft2_header_t*)NotoSansJP_Regular_10_nft2);
    nft2_unpack((nft2_header_t*)NotoSansJP_Medium_10_nft2);
    nft2_unpack((nft2_header_t*)NotoSansJP_Medium_10_clock_nft2);
    nft2_unpack((nft2_header_t*)NotoSansJP_Medium_11_nft2);
    nft2_unpack((nft2_header_t*)NotoSansJP_Medium_7_5_nft2);

    gProcessManager.Goto<App>();
    gProcessManager.MainLoop();
    return 0;
}