#include "common.h"
#include <string.h>
#include "PicoLoaderCheatDataFactory.h"

pload_cheats_t* PicoLoaderCheatDataFactory::CreateCheatData(const std::unique_ptr<GameCheats>& gameCheats) const
{
    pload_cheats_t* cheatData = nullptr;
    if (gameCheats)
    {
        u32 totalNumberOfCheats = 0;
        u32 requiredSize = GetCheatCategoryRequiredSize(gameCheats.get(), totalNumberOfCheats);
        if (totalNumberOfCheats != 0)
        {
            requiredSize += sizeof(u32) * 2;
            cheatData = (pload_cheats_t*)new u8[requiredSize];
            cheatData->length = requiredSize;
            cheatData->numberOfCheats = totalNumberOfCheats;
            u8* buffer = (u8*)&cheatData->firstCheat;
            GetCheatCategoryData(gameCheats.get(), buffer);
        }
    }

    return cheatData;
}

u32 PicoLoaderCheatDataFactory::GetCheatCategoryRequiredSize(const ICheatCategory* cheatCategory, u32& totalNumberOfCheats) const
{
    u32 requiredSize = 0;

    u32 numberOfCategories = 0;
    auto subCategories = cheatCategory->GetCategories(numberOfCategories);
    for (u32 i = 0; i < numberOfCategories; i++)
    {
        requiredSize += GetCheatCategoryRequiredSize(&subCategories[i], totalNumberOfCheats);
    }

    u32 numberOfCheats = 0;
    auto cheats = cheatCategory->GetCheats(numberOfCheats);
    for (u32 i = 0; i < numberOfCheats; i++)
    {
        u32 cheatRequiredSize = GetCheatRequiredSize(&cheats[i]);
        if (cheatRequiredSize != 0)
        {
            requiredSize += cheatRequiredSize;
            totalNumberOfCheats++;
        }
    }

    return requiredSize;
}

u32 PicoLoaderCheatDataFactory::GetCheatRequiredSize(const Cheat* cheat) const
{
    if (cheat->GetIsCheatActive())
    {
        u32 cheatDataLength = 0;
        cheat->GetCheatData(cheatDataLength);
        return sizeof(u32) + ((cheatDataLength + 7) & ~7);
    }
    else
    {
        return 0;
    }
}

void PicoLoaderCheatDataFactory::GetCheatCategoryData(const ICheatCategory* cheatCategory, u8*& buffer) const
{
    u32 numberOfCategories = 0;
    auto subCategories = cheatCategory->GetCategories(numberOfCategories);
    for (u32 i = 0; i < numberOfCategories; i++)
    {
        GetCheatCategoryData(&subCategories[i], buffer);
    }

    u32 numberOfCheats = 0;
    auto cheats = cheatCategory->GetCheats(numberOfCheats);
    for (u32 i = 0; i < numberOfCheats; i++)
    {
        GetCheatData(&cheats[i], buffer);
    }
}

void PicoLoaderCheatDataFactory::GetCheatData(const Cheat* cheat, u8*& buffer) const
{
    if (cheat->GetIsCheatActive())
    {
        u32 cheatDataLength = 0;
        auto cheatData = cheat->GetCheatData(cheatDataLength);
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
