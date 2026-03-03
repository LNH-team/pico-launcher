#include "common.h"
#include <algorithm>
#include <string.h>
#include "fat/File.h"
#include "UsrCheatDat.h"
#include "UsrCheatRepositoryFactory.h"

std::unique_ptr<UsrCheatRepository> UsrCheatRepositoryFactory::FromUsrCheatDat(const TCHAR* usrCheatDatPath)
{
    auto file = std::make_unique<File>();
    if (file->Open(usrCheatDatPath, FA_READ | FA_WRITE) != FR_OK)
    {
        LOG_ERROR("Failed to open %s\n", usrCheatDatPath);
        return nullptr;
    }

    char header[12];
    if (!file->ReadExact(header, sizeof(header)))
    {
        LOG_ERROR("Failed to read usr cheat header\n");
        return nullptr;
    }

    if (memcmp(header, "R4 CheatCode", sizeof(header)))
    {
        LOG_ERROR("Invalid usr cheat header\n");
        return nullptr;
    }

    // Read index
    if (file->Seek(0x100) != FR_OK)
    {
        LOG_ERROR("Failed to seek to usr cheat index\n");
        return nullptr;
    }

    usr_cheat_index_entry_t firstEntry;
    if (!file->ReadExact(&firstEntry, sizeof(firstEntry)))
    {
        LOG_ERROR("Failed to read first index\n");
        return nullptr;
    }

    u32 numberOfIndices = 0;
    if (firstEntry.offset != 0)
    {
        numberOfIndices = (firstEntry.offset - 0x100 - sizeof(usr_cheat_index_entry_t)) / sizeof(usr_cheat_index_entry_t);
    }

    if (file->Seek(0x100) != FR_OK)
    {
        LOG_ERROR("Failed to seek to usr cheat index\n");
        return nullptr;
    }

    auto indices = std::make_unique_for_overwrite<usr_cheat_index_entry_t[]>(numberOfIndices);
    if (!file->ReadExact(indices.get(), numberOfIndices * sizeof(usr_cheat_index_entry_t)))
    {
        LOG_ERROR("Failed to read first index\n");
        return nullptr;
    }

    // Set padding field to the size of the cheat data
    if (numberOfIndices > 0)
    {
        for (u32 i = 0; i < numberOfIndices - 1; i++)
        {
            indices[i].padding = indices[i + 1].offset - indices[i].offset;
        }

        indices[numberOfIndices - 1].padding = file->GetSize() - indices[numberOfIndices - 1].offset;
    }

    // sort by gameCode, then by headerCrc32
    std::sort(indices.get(), indices.get() + numberOfIndices,
        [] (const usr_cheat_index_entry_t& a, const usr_cheat_index_entry_t& b)
        {
            if (a.gameCode != b.gameCode)
            {
                return a.gameCode < b.gameCode;
            }

            return a.headerCrc32 < b.headerCrc32;
        });

    return std::make_unique<UsrCheatRepository>(std::move(file), std::move(indices), numberOfIndices);
}
