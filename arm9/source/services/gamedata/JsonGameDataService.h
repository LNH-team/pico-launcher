#pragma once
#include <memory>
#include "IGameDataService.h"

class JsonGameDataService : public IGameDataService
{
public:
    JsonGameDataService();

    const GameDataEntry* GetEntry(const char* fileName) const override;
    void ToggleFavorite(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) override;
    void ToggleCompleted(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) override;
    bool BackfillPath(const char* fileName, const char* fullPath) override;
    bool HasUnpathedFlaggedEntry() const override;
    void RecordLaunch(const char* fileName, const char* gameCode,
        const char* fullPath, const char* lastPlayedDateTime) override;
    void SetGameMetadata(const char* fileName, const char* name,
        const char* publisher, const char* developer) override;
    void RemoveEntry(const char* fileName) override;
    bool CloseOpenSession(const char* nowDateTime) override;
    bool DiscardOpenSession() override;
    u32 GetEntryCount() const override { return _entryCount; }
    const GameDataEntry& GetEntryByIndex(u32 index) const override { return _entries[index]; }
    u32 GetVersion() const override { return _version; }
    void SaveAsync(TaskQueueBase* ioTaskQueue) override;

private:
    std::unique_ptr<GameDataEntry[]> _entries;
    u32 _entryCount = 0;
    u32 _entryCapacity = 0;
    u32 _version = 0;
    /// @brief Set when the data file was present at boot but could not be read
    ///        or parsed. Saving is then refused: writing out an empty or partial
    ///        set would overwrite the file and destroy every mark and play stat
    ///        it still holds.
    bool _loadFailed = false;
    String<char, 96> _sessionGameFileName;
    String<char, 8> _sessionGameCode;
    String<char, 20> _sessionStart;

    GameDataEntry* Find(const char* fileName);
    /// @brief Returns nullptr when the name cannot be stored (too long), so
    ///        callers refuse the operation instead of writing a truncated key.
    GameDataEntry* GetOrCreateEntry(const char* fileName, const char* gameCode);
    void ClearSession();
    void Load();
};
