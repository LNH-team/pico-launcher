#pragma once
#include <memory>
#include "GameCheats.h"
#include "picoLoader7.h"

class PicoLoaderCheatDataFactory
{
public:
    pload_cheats_t* CreateCheatData(const std::unique_ptr<GameCheats>& gameCheats) const;

private:
    u32 GetCheatEntryRequiredSize(const CheatEntry* cheatEntry, u32& totalNumberOfCheats) const;
    void GetCheatEntryData(const CheatEntry* cheatEntry, u8*& buffer) const;
};
