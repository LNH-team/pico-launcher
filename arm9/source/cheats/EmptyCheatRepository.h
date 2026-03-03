#pragma once
#include "ICheatRepository.h"

class EmptyCheatRepository : public ICheatRepository
{
public:
    std::unique_ptr<GameCheats> GetCheatsForGame(const FastFileRef& romFile) const override
    {
        return nullptr;
    }

    void UpdateEnabledCheatsForGame(const std::unique_ptr<GameCheats>& cheats) const override
    {
        // Do nothing
    }
};
