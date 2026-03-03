#include "common.h"
#include <nds/arm9/video.h>
#include <nds/arm9/cache.h>
#include "core/Environment.h"
#include <string.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/dma/dmaTwl.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include "ipcChannels.h"
#include "fat/File.h"
#include "core/StringUtil.h"
#include "picoLoaderBootstrap.h"

#define PICO_LOADER_9_PATH    "/_pico/picoLoader9.bin"
#define PICO_LOADER_7_PATH    "/_pico/picoLoader7.bin"

typedef void (*pico_loader_9_func_t)(void);

static pload_params_t sLoadParams;
static char sLauncherPath[256] alignas(32);
static PicoLoaderBootDrive sBootDrive;
static const pload_cheats_t* sCheatData = nullptr;

pload_params_t* pload_getLoadParams()
{
    return &sLoadParams;
}

void pload_setBootDrive(PicoLoaderBootDrive bootDrive)
{
    sBootDrive = bootDrive;
}

void pload_setLauncherPath(const char* launcherPath)
{
    StringUtil::Copy(sLauncherPath, launcherPath, sizeof(sLauncherPath));
}

void pload_setCheatData(const pload_cheats_t* cheatData)
{
    sCheatData = cheatData;
}

void pload_start()
{
    mem_setVramAMapping(MEM_VRAM_AB_LCDC);
    mem_setVramCMapping(MEM_VRAM_C_LCDC);
    mem_setVramDMapping(MEM_VRAM_D_LCDC);

    auto file = new File();

    file->Open(PICO_LOADER_9_PATH, FA_OPEN_EXISTING | FA_READ);
    u32 picoLoader9Size = file->GetSize();
    auto picoLoader9 = new(cache_align) u8[picoLoader9Size];
    u32 bytesRead;
    file->Read(picoLoader9, picoLoader9Size, bytesRead);
    file->Close();
    DC_FlushRange(picoLoader9, picoLoader9Size);
    dma_ntrCopy16(3, picoLoader9, (void*)0x06800000, (picoLoader9Size + 1) & ~1);
    delete[] picoLoader9;

    file->Open(PICO_LOADER_7_PATH, FA_OPEN_EXISTING | FA_READ);
    u32 picoLoader7Size = file->GetSize();
    auto picoLoader7 = new(cache_align) u8[picoLoader7Size];
    file->Read(picoLoader7, picoLoader7Size, bytesRead);
    file->Close();
    delete file;
    DC_FlushRange(picoLoader7, picoLoader7Size);
    dma_ntrCopy16(3, picoLoader7, (void*)0x06840000, (picoLoader7Size + 1) & ~1);
    delete[] picoLoader7;

    rtos_disableIrqs();
    REG_IME = 0;

    dma_ntrStopSafe(0);
    dma_ntrStopSafe(1);
    dma_ntrStopSafe(2);
    dma_ntrStopSafe(3);

    if (Environment::IsDsiMode())
    {
        REG_NDMA0CNT = 0;
        REG_NDMA1CNT = 0;
        REG_NDMA2CNT = 0;
        REG_NDMA3CNT = 0;
    }

    DC_FlushAll();
    DC_InvalidateAll();
    IC_InvalidateAll();

    auto header = (pload_header7_t*)0x06840000;
    header->bootDrive = sBootDrive;
    dma_ntrCopy16(3, &sLoadParams, &header->loadParams, sizeof(pload_params_t));
    if (header->apiVersion >= 2)
    {
        dma_ntrCopy16(3, &sLauncherPath, &header->v2.launcherPath, sizeof(header->v2.launcherPath));
    }
    if (header->apiVersion >= 3)
    {
        header->v3.cheats = sCheatData;
    }
    mem_setVramCMapping(MEM_VRAM_C_ARM7_00000);
    mem_setVramDMapping(MEM_VRAM_D_ARM7_20000);
    ipc_sendFifoMessage(IPC_CHANNEL_LOADER, 1);
    ((pico_loader_9_func_t)0x06800000)();
}