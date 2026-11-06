#pragma once
#include "GameDataEntry.h"

class TaskQueueBase;

class IGameDataService
{
public:
    virtual ~IGameDataService() = 0;

    /// @brief Returns the entry for the given rom file, or nullptr when none
    ///        exists. An entry belongs to ONE file and is resolved by file name
    ///        only - deliberately: the browser filter walks a raw directory
    ///        listing and has no game codes to offer, so any lookup that could
    ///        also match by code would answer differently there than here and
    ///        the two views would disagree (that was issue #7). Read-only, and
    ///        never allocates: the empty-folder probe calls this from the IO
    ///        thread. The pointer is only valid until the next mutation
    ///        (compare GetVersion to detect those).
    virtual const GameDataEntry* GetEntry(const char* fileName) const = 0;

    /// @brief Marks resolve the entry by file name, so they follow the file and
    ///        never leak onto another copy of the same game. \p gameCode is
    ///        stored on the entry as metadata only and is NEVER used to find
    ///        it. When fullPath is given it is stored too, so lists that
    ///        navigate to a game (favorites, recents) work for games that were
    ///        marked but never launched.
    virtual void ToggleFavorite(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) = 0;
    virtual void ToggleCompleted(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) = 0;

    /// @brief Stores fullPath on an EXISTING flagged entry that has none yet,
    ///        so favorites/completed marks made before path recording become
    ///        navigable from the panels. Only touches entries with favorite
    ///        or completed set and an empty path. Returns true when it wrote
    ///        (the caller should then save).
    virtual bool BackfillPath(const char* fileName, const char* fullPath) = 0;

    /// @brief Whether any favorite/completed entry still lacks a path. Cheap
    ///        gate so the per-folder backfill scan is skipped once every mark
    ///        is navigable (the steady state on a large library).
    virtual bool HasUnpathedFlaggedEntry() const = 0;
    virtual void RecordLaunch(const char* fileName, const char* gameCode,
        const char* fullPath, const char* lastPlayedDateTime) = 0;

    /// @brief Replaces the user's display name, publisher and developer for a
    ///        file. Any of them may be empty, which clears that field; all
    ///        three empty leaves an entry with nothing but history. The strings
    ///        are copied, so the caller's buffers are not kept. Refuses names
    ///        that cannot be stored (see kMaxFileNameLength).
    virtual void SetGameMetadata(const char* fileName, const char* name,
        const char* publisher, const char* developer) = 0;

    /// @brief Drops the entry for a deleted rom file. By file name, matching how
    ///        marks are stored.
    virtual void RemoveEntry(const char* fileName) = 0;

    /// @brief Credits the session opened by the last RecordLaunch with the
    ///        time elapsed until now (the next launcher boot). Sessions over
    ///        6 hours are discarded: that is a power-off, not a play session.
    /// @return True when play time was credited (the caller should save).
    virtual bool CloseOpenSession(const char* nowDateTime) = 0;

    /// @brief Forgets the session opened by the last RecordLaunch without
    ///        crediting it, for when launch tracking was switched off since.
    /// @return True when there was a session to forget (the caller should save).
    virtual bool DiscardOpenSession() = 0;

    /// @brief Unordered access to all entries, for building derived lists
    ///        (e.g. recents). Indices are only valid until the next mutation.
    virtual u32 GetEntryCount() const = 0;
    virtual const GameDataEntry& GetEntryByIndex(u32 index) const = 0;

    /// @brief Incremented on every mutation; cheap to poll for UI refresh.
    virtual u32 GetVersion() const = 0;

    /// @brief Serializes on the calling thread (so concurrent mutations can't
    ///        invalidate the data being written) and enqueues the SD write on
    ///        the given task queue. Refuses to write when the data file existed
    ///        at boot but could not be loaded, since a full overwrite would
    ///        then destroy data this run never saw.
    virtual void SaveAsync(TaskQueueBase* ioTaskQueue) = 0;

    /// @brief The longest rom file name that can be stored. Longer names are
    ///        refused outright rather than silently truncated into a key that
    ///        collides with a same-prefixed file.
    static constexpr u32 kMaxFileNameLength = 96;
};

inline IGameDataService::~IGameDataService() { }
