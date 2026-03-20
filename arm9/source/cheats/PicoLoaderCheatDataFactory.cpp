#include "common.h"
#include <string.h>
#include "PicoLoaderCheatDataFactory.h"

pload_cheats_t* PicoLoaderCheatDataFactory::CreateCheatData(const std::unique_ptr<GameCheats>& gameCheats) const
{
    pload_cheats_t* cheatData = nullptr;
    if (gameCheats)
    {
        u32 totalNumberOfCheats = 0;
        u32 requiredSize = GetCheatEntryRequiredSize(gameCheats.get(), totalNumberOfCheats);
        if (totalNumberOfCheats != 0)
        {
            requiredSize += sizeof(u32) * 2;
            cheatData = (pload_cheats_t*)new u8[requiredSize];
            cheatData->length = requiredSize;
            cheatData->numberOfCheats = totalNumberOfCheats;
            u8* buffer = (u8*)&cheatData->firstCheat;
            GetCheatEntryData(gameCheats.get(), buffer);
        }
    }

    return cheatData;
}

u32 PicoLoaderCheatDataFactory::GetCheatEntryRequiredSize(const CheatEntry* cheatEntry, u32& totalNumberOfCheats) const
{
    u32 requiredSize = 0;
    if (cheatEntry->IsCheatCategory())
    {
        u32 numberOfSubEntries = 0;
        auto subEntries = cheatEntry->GetSubEntries(numberOfSubEntries);
        for (u32 i = 0; i < numberOfSubEntries; i++)
        {
            requiredSize += GetCheatEntryRequiredSize(&subEntries[i], totalNumberOfCheats);
        }
    }
    else
    {
        if (cheatEntry->GetIsCheatActive())
        {
            u32 cheatDataLength = 0;
            cheatEntry->GetCheatData(cheatDataLength);
            requiredSize += sizeof(u32) + ((cheatDataLength + 7) & ~7);
            totalNumberOfCheats++;
        }
    }

    return requiredSize;
}

void PicoLoaderCheatDataFactory::GetCheatEntryData(const CheatEntry* cheatEntry, u8*& buffer) const
{
    if (cheatEntry->IsCheatCategory())
    {
        u32 numberOfSubEntries = 0;
        auto subEntries = cheatEntry->GetSubEntries(numberOfSubEntries);
        for (u32 i = 0; i < numberOfSubEntries; i++)
        {
            GetCheatEntryData(&subEntries[i], buffer);
        }
    }
    else
    {
        if (cheatEntry->GetIsCheatActive())
        {
            u32 cheatDataLength = 0;
            auto cheatData = cheatEntry->GetCheatData(cheatDataLength);
            u32 paddedCheatDataLength = (cheatDataLength + 7) & ~7;
            *(u32*)buffer = paddedCheatDataLength;
            buffer += sizeof(u32);
            memcpy(buffer, cheatData, cheatDataLength);
            if (cheatDataLength != paddedCheatDataLength)
            {
                memset(buffer + cheatDataLength, 0, paddedCheatDataLength - cheatDataLength);
            }
            buffer += paddedCheatDataLength;
        }
    }
}
