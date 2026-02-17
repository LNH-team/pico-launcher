#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "CheatSaveManager.h"

#define CHEATS_DIR "/_pico/extras/cheats"
#define CHEATS_SAVE_MAGIC 0x43485453  // "CHTS"

void CheatSaveManager::EnsureDirectory()
{
    f_mkdir("/_pico");
    f_mkdir("/_pico/extras");
    f_mkdir(CHEATS_DIR);
}

void CheatSaveManager::BuildSavePath(const char* gameCode, const char* romFileName,
    char* outPath, u32 outPathSize)
{
    // Strip extension from romFileName
    char baseName[128];
    strncpy(baseName, romFileName, sizeof(baseName) - 1);
    baseName[sizeof(baseName) - 1] = 0;
    char* dot = strrchr(baseName, '.');
    if (dot) *dot = 0;

    mini_snprintf(outPath, outPathSize, "%s/%s_%s.dat", CHEATS_DIR, gameCode, baseName);
}

// Function adapted from TWiLightMenu
bool CheatSaveManager::WriteLinkerFormat(const CheatCodelist& cheatList, const char* gameCode, const char* romFileName)
{
    if (!gameCode || !gameCode[0] || !romFileName || !romFileName[0]) 
        return false;
    
    EnsureDirectory();
    
    char path[256];
    BuildSavePath(gameCode, romFileName, path, sizeof(path));
    
    // Change extension to .cc
    char* ext = strrchr(path, '.');
    if (ext) 
        strcpy(ext, ".cc");
    else 
        strcat(path, ".cc");

    File file;
    if (file.Open(path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        return false;
    }
    
    u32 bytesWritten;
    // Get ALL enabled cheat codes
    const auto& items = cheatList.GetItems();
    for (int i = 0; i < items.size(); ++i)
    {
        if ((items[i].flags & CheatItem::ESelected) && items[i].cheatCodeCount > 0)
        {
            file.Write(items[i].cheatCodes, items[i].cheatCodeCount * 4, bytesWritten);
        }
    }
    
    // Write terminator
    u8 term[4] = {0, 0, 0, 0xCF};
    file.Write(term, 4, bytesWritten);
    
    file.Sync();
    return true;
}

bool CheatSaveManager::SaveSelections(const CheatCodelist& cheatList, const char* gameCode, const char* romFileName)
{
    if (!gameCode || !gameCode[0] || !romFileName || !romFileName[0])
        return false;
        
    EnsureDirectory();

    char path[256];
    BuildSavePath(gameCode, romFileName, path, sizeof(path));

    File file;
    if (file.Open(path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        return false;
    }

    u32 bytesWritten;
    const auto& items = cheatList.GetItems();

    // Write magic
    u32 magic = CHEATS_SAVE_MAGIC;
    file.Write(&magic, 4, bytesWritten);

    // Write item count
    u32 count = (u32)items.size();
    file.Write(&count, 4, bytesWritten);

    // Write selection flags for each item 
    for (int i = 0; i < items.size(); i++)
    {
        u8 selected = items[i].IsSelected() ? 1 : 0;
        file.Write(&selected, 1, bytesWritten);
    }

    // Write open/close state for folders
    for (int i = 0; i < items.size(); i++)
    {
        u8 open = items[i].IsOpen() ? 1 : 0;
        file.Write(&open, 1, bytesWritten);
    }

    file.Sync();
    
    // Also write the linker format (.cc file) with the selected cheats
    WriteLinkerFormat(cheatList, gameCode, romFileName);
    
    return true;
}

bool CheatSaveManager::LoadSelections(CheatCodelist& cheatList, const char* gameCode, const char* romFileName)
{
    if (!gameCode || !romFileName)
        return false;

    char path[256];
    BuildSavePath(gameCode, romFileName, path, sizeof(path));

    File file;
    if (file.Open(path, FA_READ) != FR_OK)
        return false;

    u32 bytesRead;

    // Check magic
    u32 magic;
    if (file.Read(&magic, 4, bytesRead) != FR_OK || magic != CHEATS_SAVE_MAGIC)
        return false;

    // Read item count
    u32 count;
    if (file.Read(&count, 4, bytesRead) != FR_OK)
        return false;

    auto& items = cheatList.GetItems();
    if (count != (u32)items.size())
    {
        return false;
    }

    // Read selection flags
    for (u32 i = 0; i < count; ++i)
    {
        u8 selected;
        if (file.Read(&selected, 1, bytesRead) != FR_OK)
            return false;
        items[i].SetSelected(selected != 0);
    }

    // Read open/close state for folders
    for (u32 i = 0; i < count; ++i)
    {
        u8 open;
        if (file.Read(&open, 1, bytesRead) != FR_OK)
            break; 
        if (items[i].IsFolder())
        {
            if (open) items[i].flags |= CheatItem::EOpen;
            else items[i].flags &= ~CheatItem::EOpen;
        }
    }

    cheatList.BuildVisibleList();
    return true;
}

bool CheatSaveManager::WriteCheatsToFile(const CheatCodelist& cheatList, const char* outputPath)
{
    u32 codeCount = cheatList.GetSelectedCheatCodeCount();
    if (codeCount == 0)
        return false;

    u32* codes = (u32*)malloc(codeCount * sizeof(u32));
    if (!codes)
        return false;

    cheatList.CopySelectedCheats(codes);

    File file;
    if (file.Open(outputPath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        free(codes);
        return false;
    }

    u32 bytesWritten;
    file.Write(codes, codeCount * sizeof(u32), bytesWritten);
    free(codes);

    // Write terminator (0xCF000000)
    u32 terminator = 0xCF000000;
    file.Write(&terminator, 4, bytesWritten);

    file.Sync();
    return true;
}
