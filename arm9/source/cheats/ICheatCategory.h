#pragma once

class CheatCategory;
class Cheat;

class ICheatCategory
{
public:
    virtual ~ICheatCategory() = default;

    virtual bool GetIsMaxOneCheatActive() const = 0;
    virtual const CheatCategory* GetCategories(u32& numberOfCategories) const;
    virtual const Cheat* GetCheats(u32& numberOfCheats) const;

protected:
    ICheatCategory() = default;
};
