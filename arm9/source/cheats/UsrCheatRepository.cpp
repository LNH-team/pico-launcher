#include "common.h"
#include <algorithm>
#include <string.h>
#include "UsrCheatRepository.h"

#define CRCPOLY 0xEDB88320

static u32 crc32(const void* buffer, u32 length)
{
    u32 crc = ~0u;
    const u8* p = (u8*)buffer;
    while (length--)
    {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY : 0);
        }
    }

    return crc;
}

std::unique_ptr<GameCheats> UsrCheatRepository::GetCheatsForGame(const FastFileRef& romFile) const
{
    auto file = std::make_unique<File>();
    file->Open(romFile, FA_READ);
    auto headerBuffer = std::make_unique_for_overwrite<u8[]>(512);
    if (!file->ReadExact(headerBuffer.get(), 512))
    {
        LOG_ERROR("Could not read rom header\n");
        return nullptr;
    }
    file->Close();

    u32 gameCode = *(u32*)(headerBuffer.get() + 0xC);
    u32 crc = crc32(headerBuffer.get(), 512);

    headerBuffer.reset();

    return GetCheatsForGame(gameCode, crc);
}

void UsrCheatRepository::UpdateEnabledCheatsForGame(const std::unique_ptr<GameCheats>& cheats) const
{
    u32 cheatDataLength = 0;
    auto cheatData = cheats->GetCheatData(cheatDataLength);
    if (_usrCheatFile->Seek(cheats->GetFileOffset()) != FR_OK)
    {
        LOG_ERROR("Failed to seek to cheat data\n");
        return;
    }

    u32 bytesWritten = 0;
    if (_usrCheatFile->Write(cheatData, cheatDataLength, bytesWritten) != FR_OK ||
        bytesWritten != cheatDataLength)
    {
        LOG_ERROR("Failed to write cheat data\n");
        return;
    }

    if (_usrCheatFile->Sync() != FR_OK)
    {
        LOG_ERROR("Failed to flush cheat data\n");
    }
}

std::unique_ptr<GameCheats> UsrCheatRepository::GetCheatsForGame(u32 gameCode, u32 headerCrc32) const
{
    auto index = FindIndex(gameCode, headerCrc32);
    if (index == nullptr)
    {
        LOG_DEBUG("Cheats not found for %c%c%c%c - 0x%X\n",
            gameCode & 0xFF, (gameCode >> 8) & 0xFF, (gameCode >> 16) & 0xFF, gameCode >> 24,
            headerCrc32);
        return nullptr;
    }

    const u32 cheatDataLength = index->padding; // padding was set to the size in UsrCheatRepositoryFactory
    auto cheatData = std::make_unique_for_overwrite<u8[]>(cheatDataLength);
    if (_usrCheatFile->Seek(index->offset) != FR_OK)
    {
        LOG_ERROR("Failed to seek to cheat data\n");
        return nullptr;
    }
    if (!_usrCheatFile->ReadExact(cheatData.get(), cheatDataLength))
    {
        LOG_ERROR("Failed to read cheat data\n");
        return nullptr;
    }

    u8* ptr = cheatData.get();

    // game name
    const char* gameName = (const char*)ptr;
    ptr += strlen(gameName) + 1;

    // padding
    ptr = (u8*)(((u32)ptr + 3) & ~3); // 32-bit align

    // flags
    u32 flags = *(u32*)ptr;
    u32 totalNumberOfItems = flags & 0x0FFFFFFF;
    // u32 gameActive = flags >> 28;
    ptr += 4;

    // master codes
    ptr += 8 * 4;

    auto entries = new CheatEntry[totalNumberOfItems];
    u32 entryCount = 0;

    while (ptr < cheatData.get() + cheatDataLength && entryCount < totalNumberOfItems)
    {
        u32 itemFlags = *(u32*)ptr;
        bool isCategory = ((itemFlags >> 28) & 1) == 1;
        if (isCategory)
        {
            entries[entryCount++] = ParseCategory(ptr);
        }
        else
        {
            entries[entryCount++] = ParseCheat(ptr);
        }
    }

    auto actualEntries = new CheatEntry[entryCount];
    std::move(entries, entries + entryCount, actualEntries);
    delete[] entries;

    return std::make_unique<GameCheats>(
        std::move(cheatData), cheatDataLength, index->offset, gameName, actualEntries, entryCount);
}

const usr_cheat_index_entry_t* UsrCheatRepository::FindIndex(u32 gameCode, u32 headerCrc32) const
{
    if (_numberOfIndices != 0)
    {
        const auto index = std::lower_bound(_sortedIndices.get(), _sortedIndices.get() + _numberOfIndices, gameCode,
            [headerCrc32] (const usr_cheat_index_entry_t& entry, u32 value)
            {
                if (entry.gameCode != value)
                {
                    return entry.gameCode < value;
                }

                return entry.headerCrc32 < headerCrc32;
            });

        if (index != _sortedIndices.get() + _numberOfIndices &&
            index->gameCode == gameCode &&
            index->headerCrc32 == headerCrc32)
        {
            return index;
        }
    }

    return nullptr;
}

CheatEntry UsrCheatRepository::ParseCategory(u8*& ptr) const
{
    // flags
    u32 itemFlags = *(u32*)ptr;
    ptr += 4;
    u32 numberOfItems = itemFlags & 0x00FFFFFF;
    bool isMaxOneCheatActive = ((itemFlags >> 24) & 1) == 1;

    // item name
    const char* itemName = (const char*)ptr;
    ptr += strlen(itemName) + 1;

    // item description
    const char* itemDescription = (const char*)ptr;
    ptr += strlen(itemDescription) + 1;

    // padding
    ptr = (u8*)(((u32)ptr + 3) & ~3); // 32-bit align

    auto entries = new CheatEntry[numberOfItems];
    for (u32 i = 0; i < numberOfItems; i++)
    {
        u32 itemFlags = *(u32*)ptr;
        bool isCategory = ((itemFlags >> 28) & 1) == 1;
        if (isCategory)
        {
            entries[i] = ParseCategory(ptr);
        }
        else
        {
            entries[i] = ParseCheat(ptr);
        }
    }

    return CheatEntry(itemName, itemDescription, isMaxOneCheatActive, entries, numberOfItems);
}

CheatEntry UsrCheatRepository::ParseCheat(u8*& ptr) const
{
    // flags
    u32* flagsPtr = (u32*)ptr;
    ptr += 4;

    // item name
    const char* itemName = (const char*)ptr;
    ptr += strlen(itemName) + 1;

    // item description
    const char* itemDescription = (const char*)ptr;
    ptr += strlen(itemDescription) + 1;

    // padding
    ptr = (u8*)(((u32)ptr + 3) & ~3); // 32-bit align

    // number of code words
    u32 numberOfCodeWords = *(u32*)ptr;
    ptr += 4;

    const void* cheatData = ptr;

    // code
    ptr += numberOfCodeWords * 4;

    return CheatEntry(itemName, itemDescription, flagsPtr, cheatData, numberOfCodeWords * 4);
}
