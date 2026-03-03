#pragma once
#include <memory>
#include "CheatTreeItem.h"
#include "Cheat.h"
#include "ICheatCategory.h"

class CheatCategory : public CheatTreeItem, public ICheatCategory
{
public:
    CheatCategory()
        : _isMaxOneCheatActive(false), _subCategories(nullptr), _numberOfSubCategories(0)
        , _cheats(nullptr), _numberOfCheats(0) { }

    CheatCategory(const char* name, const char* description, bool isMaxOneCheatActive,
        CheatCategory* subCategories, u32 numberOfSubCategories, Cheat* cheats, u32 numberOfCheats)
        : CheatTreeItem(name, description), _isMaxOneCheatActive(isMaxOneCheatActive)
        , _subCategories(subCategories), _numberOfSubCategories(numberOfSubCategories)
        , _cheats(cheats), _numberOfCheats(numberOfCheats) { }

    CheatCategory(CheatCategory& other) = delete;
    CheatCategory& operator=(CheatCategory& other) = delete;

    CheatCategory(CheatCategory&& other)
    {
        *this = std::move(other);
    }

    CheatCategory& operator=(CheatCategory&& other)
    {
        _name = other._name;
        other._name = nullptr;
        _description = other._description;
        other._description = nullptr;
        _isMaxOneCheatActive = other._isMaxOneCheatActive;
        other._isMaxOneCheatActive = false;
        _subCategories = other._subCategories;
        other._subCategories = nullptr;
        _numberOfSubCategories = other._numberOfSubCategories;
        other._numberOfSubCategories = 0;
        _cheats = other._cheats;
        other._cheats = nullptr;
        _numberOfCheats = other._numberOfCheats;
        other._numberOfCheats = 0;
        return *this;
    }

    ~CheatCategory()
    {
        if (_subCategories != nullptr)
        {
            free(_subCategories);
        }
        if (_cheats != nullptr)
        {
            free(_cheats);
        }
    }

    bool GetIsMaxOneCheatActive() const override
    {
        return _isMaxOneCheatActive;
    }

    const CheatCategory* GetCategories(u32& numberOfCategories) const override
    {
        numberOfCategories = _numberOfSubCategories;
        return _subCategories;
    }

    const Cheat* GetCheats(u32& numberOfCheats) const override
    {
        numberOfCheats = _numberOfCheats;
        return _cheats;
    }

private:
    bool _isMaxOneCheatActive;
    CheatCategory* _subCategories;
    u32 _numberOfSubCategories;
    Cheat* _cheats;
    u32 _numberOfCheats;
};
