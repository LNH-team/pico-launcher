#pragma once
#include <memory>
#include "GameCheats.h"
#include "picoLoader7.h"

class PicoLoaderCheatDataFactory
{
public:
    pload_cheats_t* CreateCheatData(const std::unique_ptr<GameCheats>& gameCheats) const;

private:
    u32 GetCheatCategoryRequiredSize(const ICheatCategory* cheatCategory, u32& totalNumberOfCheats) const;
    u32 GetCheatRequiredSize(const Cheat* cheat) const;
    void GetCheatCategoryData(const ICheatCategory* cheatCategory, u8*& buffer) const;
    void GetCheatData(const Cheat* cheat, u8*& buffer) const;
};
