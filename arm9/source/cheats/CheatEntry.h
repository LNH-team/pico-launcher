#pragma once

/// @brief Class representing a cheat or a cheat category.
class CheatEntry
{
public:
    /// @brief Dummy empty constructor.
    ///        Produces an invalid entry, used internally to signal that parsing malformed
    ///        cheat data failed. See \see IsValid().
    CheatEntry()
        : _isCheatCategory(false), _flagsPointer(nullptr), _cheatData(nullptr), _cheatDataLength(0)
        , _isValid(false) { }

    /// @brief Constructor for a cheat.
    CheatEntry(const char* name, const char* description, u32* flagsPointer, const void* cheatData, u32 cheatDataLength)
        : _name(name), _description(description), _isCheatCategory(false), _flagsPointer(flagsPointer)
        , _cheatData(cheatData), _cheatDataLength(cheatDataLength), _isValid(true) { }

    /// @brief Constructor for a category.
    CheatEntry(const char* name, const char* description, bool isMaxOneCheatActive, CheatEntry* subEntries, u32 numberOfSubEntries)
        : _name(name), _description(description), _isCheatCategory(true), _isMaxOneCheatActive(isMaxOneCheatActive)
        , _subEntries(subEntries), _numberOfSubEntries(numberOfSubEntries), _isValid(true) { }

    CheatEntry(const CheatEntry& other) = delete;

    CheatEntry(CheatEntry&& other)
        : _isCheatCategory(false), _flagsPointer(nullptr), _cheatData(nullptr), _cheatDataLength(0)
        , _isValid(false)
    {
        *this = std::move(other);
    }

    ~CheatEntry()
    {
        if (_isCheatCategory && _subEntries != nullptr)
        {
            delete[] _subEntries;
        }
    }

    CheatEntry& operator=(const CheatEntry& other) = delete;

    CheatEntry& operator=(CheatEntry&& other)
    {
        if (_isCheatCategory && _subEntries != nullptr)
        {
            delete[] _subEntries;
            _subEntries = nullptr;
        }

        _name = other._name;
        other._name = nullptr;
        _description = other._description;
        other._description = nullptr;
        _isCheatCategory = other.IsCheatCategory();
        if (_isCheatCategory)
        {
            _isMaxOneCheatActive = other._isMaxOneCheatActive;
            _subEntries = other._subEntries;
            other._subEntries = nullptr;
            _numberOfSubEntries = other._numberOfSubEntries;
            other._numberOfSubEntries = 0;
        }
        else
        {
            _flagsPointer = other._flagsPointer;
            other._flagsPointer = nullptr;
            _cheatData = other._cheatData;
            other._cheatData = nullptr;
            _cheatDataLength = other._cheatDataLength;
            other._cheatDataLength = 0;
        }
        _isValid = other._isValid;
        other._isValid = false;

        return *this;
    }

    /// @brief Gets the name of this cheat entry.
    /// @return A pointer to the name of this cheat entry.
    const char* GetName() const
    {
        return _name;
    }

    /// @brief Gets the description of this cheat entry.
    /// @return A pointer to the description of this cheat entry.
    const char* GetDescription() const
    {
        return _description;
    }

    /// @brief When this entry is a cheat, gets a pointer to the cheat data.
    /// @param cheatDataLength The length of the cheat data is returned in this reference.
    /// @return A pointer to the cheat data.
    ///         This pointer is only valid for the lifetime of the \see GameCheats instance this cheat belongs to.
    ///         If this entry is not a cheat, \c nullptr is returned.
    const void* GetCheatData(u32& cheatDataLength) const
    {
        if (_isCheatCategory)
        {
            cheatDataLength = 0;
            return nullptr;
        }
        else
        {
            cheatDataLength = _cheatDataLength;
            return _cheatData;
        }
    }

    /// @brief Gets whether this entry is an active (enabled) cheat or not.
    /// @return \c true when this entry is an active cheat, or \c false otherwise.
    bool GetIsCheatActive() const
    {
        if (_isCheatCategory)
        {
            return false;
        }
        else
        {
            return ((*_flagsPointer >> 24) & 1) == 1;
        }
    }

    /// @brief If this entry is a cheat, sets whether this cheat is active (enabled) or not.
    /// @param isCheatActive \c true to enable this cheat, or \c false to disable this cheat.
    void SetIsCheatActive(bool isCheatActive) const
    {
        if (!_isCheatCategory)
        {
            u32 flags = *_flagsPointer;
            flags &= ~(1 << 24);
            if (isCheatActive)
            {
                flags |= 1 << 24;
            }
            *_flagsPointer = flags;
        }
    }

    /// @brief Indicates if this entry is a cheat category or not.
    /// @return \c true when this entry is a cheat category, or \c false when this entry is a cheat.
    bool IsCheatCategory() const
    {
        return _isCheatCategory;
    }

    /// @brief Indicates if this entry was parsed successfully from cheat data.
    /// @return \c true when this entry is valid and safe to use, or \c false when it is a dummy
    ///         entry produced when parsing malformed cheat data failed. Callers must not call
    ///         \see GetIsCheatActive() or \see SetIsCheatActive() on an invalid entry.
    bool IsValid() const
    {
        return _isValid;
    }

    /// @brief Indicates if this entry is a cheat category in which only one cheat is allowed to be on at a time or not.
    /// @return \c true when this entry is a cheat category in which only one cheat is allowed
    ///         to be active at a time, or \c false otherwise.
    bool GetIsMaxOneCheatActive() const
    {
        return _isCheatCategory && _isMaxOneCheatActive;
    }

    /// @brief Gets the sub-entries of this entry.
    /// @param numberOfSubEntries The number of sub-entries is returned through this reference.
    /// @return A pointer to an array of entries. This may be \c nullptr when \p numberOfSubEntries is 0.
    const CheatEntry* GetSubEntries(u32& numberOfSubEntries) const
    {
        if (_isCheatCategory)
        {
            numberOfSubEntries = _numberOfSubEntries;
            return _subEntries;
        }
        else
        {
            numberOfSubEntries = 0;
            return nullptr;
        }
    }

private:
    const char* _name = nullptr;
    const char* _description = nullptr;
    bool _isCheatCategory;

    union
    {
        struct
        {
            // For cheat
            u32* _flagsPointer;
            const void* _cheatData;
            u32 _cheatDataLength;
        };
        struct
        {
            // For category
            bool _isMaxOneCheatActive;
            CheatEntry* _subEntries;
            u32 _numberOfSubEntries;
        };
    };

    bool _isValid;
};
