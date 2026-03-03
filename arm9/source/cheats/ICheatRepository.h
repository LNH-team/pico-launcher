#pragma once
#include "GameCheats.h"
#include "fat/FastFileRef.h"

class ICheatRepository
{
public:
    virtual ~ICheatRepository() { }

    virtual std::unique_ptr<GameCheats> GetCheatsForGame(const FastFileRef& romFile) const = 0;
    virtual void UpdateEnabledCheatsForGame(const std::unique_ptr<GameCheats>& cheats) const = 0;

protected:
    ICheatRepository() { }
};
