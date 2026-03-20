#pragma once

class CheatTreeItem
{
public:
    const char* GetName() const
    {
        return _name;
    }

    const char* GetDescription() const
    {
        return _description;
    }

protected:
    const char* _name;
    const char* _description;

    CheatTreeItem()
        : _name(nullptr), _description(nullptr) { }

    CheatTreeItem(const char* name, const char* description)
        : _name(name), _description(description) { }
};
