#include "common.h"
#include <memory>
#include <string.h>
#include <algorithm>
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "core/task/TaskQueue.h"
#include "JsonGameDataService.h"

#pragma GCC optimize("Os")

#define GAME_DATA_FILE_PATH   "/_pico/gamedata.json"
// written first, then renamed over the real file, so an interrupted save cannot
// leave the live file truncated (see SaveAsync)
#define GAME_DATA_TEMP_PATH   "/_pico/gamedata.tmp"
#define KEY_GAMES             "games"
#define KEY_GAME_CODE         "gameCode"
#define KEY_FAVORITE          "favorite"
#define KEY_COMPLETED         "completed"
#define KEY_LAUNCH_COUNT      "launchCount"
#define KEY_PLAY_MINUTES      "playMinutes"
#define KEY_LAST_PLAYED       "lastPlayed"
#define KEY_DISPLAY_NAME      "name"
#define KEY_PUBLISHER         "publisher"
#define KEY_DEVELOPER         "developer"
#define KEY_PATH              "path"
#define KEY_SESSION_GAME      "sessionGame"
#define KEY_SESSION_GAME_CODE "sessionGameCode"
#define KEY_SESSION_START     "sessionStart"

#define SESSION_MAX_MINUTES   (6 * 60)

// ArduinoJson silently drops data when its pool is exhausted, so the pool is
// sized from the entry count (write) or file size (read) instead of a fixed
// JSON_RESERVED_SIZE like settings.json uses.
// The per-entry budget is generous on purpose. Strings are LINKED, not copied:
// every key and value here is a const char* that ArduinoJson stores as a pointer
// (StringStoragePolicy::Link), so an entry only costs its ~8 16-byte VariantSlots
// - on the order of 128 bytes, not the 440 reserved. Do NOT "correct" this by
// adding the fileName and path lengths: those bytes live in _entries and in the
// parse buffer, never in the pool.
#define JSON_POOL_BASE_SIZE       4096
#define JSON_POOL_PER_ENTRY       440
#define JSON_POOL_READ_MAX        (96 * 1024)

static u32 writePoolSize(u32 entryCount)
{
    return JSON_POOL_BASE_SIZE + entryCount * JSON_POOL_PER_ENTRY;
}

/// @brief Parses "YYYY-MM-DD HH:MM" into absolute minutes (days-from-civil
///        algorithm), so sessions crossing midnight or month ends work.
static bool parseDateTime(const char* text, s64& totalMinutes)
{
    if (strlen(text) < 16)
        return false;
    auto number = [] (const char* p, int digits) -> int
    {
        int value = 0;
        for (int i = 0; i < digits; i++)
        {
            if (p[i] < '0' || p[i] > '9')
                return -1;
            value = value * 10 + (p[i] - '0');
        }
        return value;
    };
    int year = number(text, 4);
    int month = number(text + 5, 2);
    int day = number(text + 8, 2);
    int hour = number(text + 11, 2);
    int minute = number(text + 14, 2);
    if (year < 2000 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        return false;
    }
    year -= month <= 2;
    int era = year / 400;
    u32 yearOfEra = year - era * 400;
    u32 dayOfYear = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    u32 dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    s64 days = (s64)era * 146097 + dayOfEra;
    totalMinutes = days * 1440 + hour * 60 + minute;
    return true;
}

// A file name at or over the storage limit would be silently truncated into a
// key that collides with any same-prefixed file: two roms would then share one
// entry in memory and, worse, collapse into a single json member on save, losing
// one of the two marks for good. Refuse those names instead.
static bool isStorableFileName(const char* fileName)
{
    // String<char, N> holds N characters plus the terminator, so a name of
    // exactly kMaxFileNameLength fits and must be accepted: refusing it would
    // drop entries that older versions stored just fine.
    return fileName && fileName[0] != 0 &&
        strlen(fileName) <= IGameDataService::kMaxFileNameLength;
}

// homebrew headers can hold garbage where retail games keep their code; only
// printable codes are worth storing (and safe inside the json file). "####" is
// the toolchain's placeholder, which carries no information at all.
// NOTE: the code is metadata only. It is never used to look an entry up - see
// the comment on IGameDataService::GetEntry.
static bool isUsableGameCode(const char* gameCode)
{
    if (!gameCode || gameCode[0] == 0)
        return false;
    bool allPlaceholder = true;
    for (const char* c = gameCode; *c != 0; c++)
    {
        if (*c < 0x20 || *c >= 0x7F)
            return false;
        if (*c != '#')
            allPlaceholder = false;
    }
    return !allPlaceholder;
}

JsonGameDataService::JsonGameDataService()
{
    Load();
}

GameDataEntry* JsonGameDataService::Find(const char* fileName)
{
    for (u32 i = 0; i < _entryCount; i++)
    {
        if (!strcasecmp(_entries[i].fileName.GetString(), fileName))
            return &_entries[i];
    }
    return nullptr;
}

const GameDataEntry* JsonGameDataService::GetEntry(const char* fileName) const
{
    // By name only, and nothing else: every view must agree on what an entry
    // belongs to. Read-only and allocation-free (the empty-folder probe calls
    // this from the IO thread).
    return const_cast<JsonGameDataService*>(this)->Find(fileName);
}

GameDataEntry* JsonGameDataService::GetOrCreateEntry(const char* fileName, const char* gameCode)
{
    if (!isStorableFileName(fileName))
    {
        LOG_ERROR("Game data: file name too long to track (%s)\n", fileName ? fileName : "");
        return nullptr;
    }

    if (auto* existing = Find(fileName))
    {
        // keep the code fresh as metadata; it never affects which entry this is
        if (isUsableGameCode(gameCode))
            existing->gameCode = gameCode;
        return existing;
    }

    if (_entryCount == _entryCapacity)
    {
        u32 newCapacity = _entryCapacity == 0 ? 16 : _entryCapacity * 2;
        auto newEntries = std::make_unique<GameDataEntry[]>(newCapacity);
        for (u32 i = 0; i < _entryCount; i++)
            newEntries[i] = _entries[i];
        _entries = std::move(newEntries);
        _entryCapacity = newCapacity;
    }

    auto& entry = _entries[_entryCount++];
    entry = GameDataEntry();
    entry.fileName = fileName;
    if (isUsableGameCode(gameCode))
        entry.gameCode = gameCode;
    return &entry;
}

void JsonGameDataService::ToggleFavorite(const char* fileName, const char* gameCode,
    const char* fullPath)
{
    auto* entry = GetOrCreateEntry(fileName, gameCode);
    if (!entry)
        return;
    entry->favorite = !entry->favorite;
    if (fullPath)
        entry->path = fullPath;
    _version++;
}

void JsonGameDataService::ToggleCompleted(const char* fileName, const char* gameCode,
    const char* fullPath)
{
    auto* entry = GetOrCreateEntry(fileName, gameCode);
    if (!entry)
        return;
    entry->completed = !entry->completed;
    if (fullPath)
        entry->path = fullPath;
    _version++;
}

bool JsonGameDataService::BackfillPath(const char* fileName, const char* fullPath)
{
    if (!fullPath || fullPath[0] == 0)
        return false;
    GameDataEntry* entry = Find(fileName);
    // only fill a still-empty path on an entry a panel would navigate to
    if (!entry || entry->path.GetString()[0] != 0 || (!entry->favorite && !entry->completed))
        return false;
    entry->path = fullPath;
    _version++;
    return true;
}

bool JsonGameDataService::HasUnpathedFlaggedEntry() const
{
    for (u32 i = 0; i < _entryCount; i++)
    {
        const auto& entry = _entries[i];
        if ((entry.favorite || entry.completed) && entry.path.GetString()[0] == 0)
            return true;
    }
    return false;
}

void JsonGameDataService::RecordLaunch(const char* fileName, const char* gameCode,
    const char* fullPath, const char* lastPlayedDateTime)
{
    auto* entry = GetOrCreateEntry(fileName, gameCode);
    if (!entry)
        return;
    entry->launchCount++;
    entry->lastPlayed = lastPlayedDateTime;
    entry->path = fullPath;
    // a play session opens now and closes at the next launcher boot
    _sessionGameFileName = fileName;
    _sessionGameCode = isUsableGameCode(gameCode) ? gameCode : "";
    _sessionStart = lastPlayedDateTime;
    _version++;
}

void JsonGameDataService::SetGameMetadata(const char* fileName, const char* name,
    const char* publisher, const char* developer)
{
    // the sheet only opens on a game, but the service is the one that must
    // refuse a name that cannot be a key
    auto* entry = GetOrCreateEntry(fileName, nullptr);
    if (!entry)
        return;
    entry->displayName = name ? name : "";
    entry->publisher = publisher ? publisher : "";
    entry->developer = developer ? developer : "";
    _version++;
}

bool JsonGameDataService::CloseOpenSession(const char* nowDateTime)
{
    if (_sessionStart.GetString()[0] == 0)
        return false;

    s64 startMinutes = 0;
    s64 nowMinutes = 0;
    if (parseDateTime(_sessionStart.GetString(), startMinutes) &&
        parseDateTime(nowDateTime, nowMinutes))
    {
        s64 minutes = nowMinutes - startMinutes;
        // longer than the cap means the console was off, not playing
        if (minutes >= 1 && minutes <= SESSION_MAX_MINUTES)
        {
            // by name, like every other lookup: if the rom was renamed since the
            // launch there is nothing to credit and the minutes are dropped
            // rather than invented onto a new entry
            if (auto* entry = Find(_sessionGameFileName.GetString()))
                entry->playMinutes += (u32)minutes;
        }
    }
    ClearSession();
    // the cleared session (and any credited time) must be persisted
    return true;
}

bool JsonGameDataService::DiscardOpenSession()
{
    if (_sessionStart.GetString()[0] == 0)
        return false;
    ClearSession();
    return true;
}

void JsonGameDataService::ClearSession()
{
    _sessionStart = "";
    _sessionGameFileName = "";
    _sessionGameCode = "";
    _version++;
}

void JsonGameDataService::RemoveEntry(const char* fileName)
{
    GameDataEntry* entry = Find(fileName);
    if (!entry)
        return;
    _entryCount--;
    *entry = _entries[_entryCount];
    _version++;
}

void JsonGameDataService::SaveAsync(TaskQueueBase* ioTaskQueue)
{
    // The file existed but this run could not read it, so memory holds at most
    // a fraction of what is on the card. A save rewrites the whole file, and the
    // prune below drops every default-valued entry, so writing now would destroy
    // marks and play time for good. Better to lose this session's changes.
    if (_loadFailed)
    {
        LOG_ERROR("Game data not saved: the existing file failed to load\n");
        return;
    }

    // serialization happens on the calling (main) thread: ArduinoJson links
    // pointers into _entries, which a concurrent mutation could reallocate if
    // this ran on the IO thread. Only the finished byte buffer crosses threads.
    DynamicJsonDocument json(writePoolSize(_entryCount));
    auto games = json[KEY_GAMES].to<JsonObject>();
    for (u32 i = 0; i < _entryCount; i++)
    {
        const auto& entry = _entries[i];
        // entries reset back to all-default state are pruned on write; the
        // edited name/credits count as data, so an entry the user renamed is
        // kept even though it was never played
        if (!entry.favorite && !entry.completed && entry.launchCount == 0 &&
            entry.playMinutes == 0 && !entry.HasDisplayMetadata())
            continue;
        auto game = games[entry.fileName.GetString()].to<JsonObject>();
        if (entry.gameCode.GetString()[0] != 0)
            game[KEY_GAME_CODE] = entry.gameCode.GetString();
        if (entry.displayName.GetString()[0] != 0)
            game[KEY_DISPLAY_NAME] = entry.displayName.GetString();
        if (entry.publisher.GetString()[0] != 0)
            game[KEY_PUBLISHER] = entry.publisher.GetString();
        if (entry.developer.GetString()[0] != 0)
            game[KEY_DEVELOPER] = entry.developer.GetString();
        if (entry.favorite)
            game[KEY_FAVORITE] = true;
        if (entry.completed)
            game[KEY_COMPLETED] = true;
        if (entry.launchCount > 0)
            game[KEY_LAUNCH_COUNT] = entry.launchCount;
        if (entry.playMinutes > 0)
            game[KEY_PLAY_MINUTES] = entry.playMinutes;
        if (entry.lastPlayed.GetString()[0] != 0)
            game[KEY_LAST_PLAYED] = entry.lastPlayed.GetString();
        if (entry.path.GetString()[0] != 0)
            game[KEY_PATH] = entry.path.GetString();
    }
    if (_sessionStart.GetString()[0] != 0)
    {
        json[KEY_SESSION_GAME] = _sessionGameFileName.GetString();
        if (_sessionGameCode.GetString()[0] != 0)
            json[KEY_SESSION_GAME_CODE] = _sessionGameCode.GetString();
        json[KEY_SESSION_START] = _sessionStart.GetString();
    }
    if (json.overflowed())
    {
        LOG_ERROR("Game data json pool exhausted, not saving\n");
        return;
    }

    u32 outputSize = measureJsonPretty(json);
    u8* fileData = new(cache_align) u8[outputSize];
    serializeJsonPretty(json, fileData, outputSize);

    ioTaskQueue->Enqueue([fileData, outputSize] (const vu8& cancelRequested)
    {
        // Write to a temporary file and only then swap it in. Opening the real
        // file with FA_CREATE_ALWAYS truncates it to zero before a single byte is
        // written, so losing power (or the card) mid-write used to leave an empty
        // file, which the next boot reads as "no data" and the next save makes
        // permanent. This way the previous file survives until a complete new one
        // exists on the card.
        bool written = false;
        {
            const auto file = std::make_unique<File>();
            if (file->Open(GAME_DATA_TEMP_PATH, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
            {
                LOG_ERROR("Couldn't open game data temp file for writing\n");
            }
            else
            {
                u32 bytesWritten;
                if (file->Write(fileData, outputSize, bytesWritten) != FR_OK || bytesWritten != outputSize)
                {
                    LOG_ERROR("Error while writing game data file\n");
                }
                // Close explicitly and check it: the flush happens here, so a
                // failure at close means the temp file is incomplete. Letting the
                // destructor do it silently would rename a short file over the
                // good one.
                else if (file->Close() != FR_OK)
                {
                    LOG_ERROR("Error while flushing game data file\n");
                }
                else
                {
                    written = true;
                }
            }
        }
        if (written)
        {
            // f_rename refuses to overwrite, so the old file goes first. The
            // window this opens is a few milliseconds wide and, unlike the
            // truncate it replaces, a complete replacement is already on the card.
            f_unlink(GAME_DATA_FILE_PATH);
            if (f_rename(GAME_DATA_TEMP_PATH, GAME_DATA_FILE_PATH) != FR_OK)
                LOG_ERROR("Couldn't swap in the new game data file\n");
            else
                LOG_DEBUG("Game data file written\n");
        }
        else
        {
            f_unlink(GAME_DATA_TEMP_PATH);
        }
        delete[] fileData;
        return TaskResult<void>::Completed();
    });
}

void JsonGameDataService::Load()
{
    // A previous run may have died partway through the swap in SaveAsync. If it
    // died AFTER the live file was unlinked, the temp file is the only copy left
    // and deleting it here would destroy everything the atomic save exists to
    // protect - so promote it instead. Only when a live file is present is the
    // leftover a stale duplicate safe to drop.
    FILINFO tempInfo;
    if (f_stat(GAME_DATA_TEMP_PATH, &tempInfo) == FR_OK)
    {
        FILINFO liveInfo;
        if (f_stat(GAME_DATA_FILE_PATH, &liveInfo) == FR_OK)
        {
            f_unlink(GAME_DATA_TEMP_PATH);
        }
        else if (f_rename(GAME_DATA_TEMP_PATH, GAME_DATA_FILE_PATH) == FR_OK)
        {
            LOG_ERROR("Recovered game data from an interrupted save\n");
        }
    }

    const auto file = std::make_unique<File>();
    FRESULT openResult = file->Open(GAME_DATA_FILE_PATH, FA_READ | FA_OPEN_EXISTING);
    if (openResult != FR_OK)
    {
        // no file yet is the normal first run; anything else means the data is
        // there but unreachable, so saving must not overwrite it
        if (openResult != FR_NO_FILE && openResult != FR_NO_PATH)
        {
            LOG_ERROR("Couldn't open game data file (%d)\n", openResult);
            _loadFailed = true;
        }
        return;
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
    {
        // nothing to lose, so leave saving enabled: an empty file is what an
        // interrupted write used to leave behind
        LOG_ERROR("Game data file is empty\n");
        return;
    }

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK)
    {
        LOG_ERROR("Couldn't read game data file\n");
        _loadFailed = true;
        return;
    }

    DynamicJsonDocument json(std::clamp<u32>(fileSize * 3, 2 * JSON_POOL_BASE_SIZE, JSON_POOL_READ_MAX));
    if (deserializeJson(json, fileData.get(), fileSize) != DeserializationError::Ok)
    {
        LOG_ERROR("Couldn't parse game data file\n");
        _loadFailed = true;
        return;
    }

    auto games = json[KEY_GAMES].as<JsonObjectConst>();
    if (games.isNull())
        return;
    for (auto item : games)
    {
        auto* entry = GetOrCreateEntry(item.key().c_str(), item.value()[KEY_GAME_CODE] | "");
        // an unstorable key (too long) is skipped rather than truncated into a
        // collision; it would be unreachable from the browser anyway
        if (!entry)
            continue;
        entry->displayName = item.value()[KEY_DISPLAY_NAME] | "";
        entry->publisher = item.value()[KEY_PUBLISHER] | "";
        entry->developer = item.value()[KEY_DEVELOPER] | "";
        entry->favorite = item.value()[KEY_FAVORITE] | false;
        entry->completed = item.value()[KEY_COMPLETED] | false;
        entry->launchCount = item.value()[KEY_LAUNCH_COUNT] | 0u;
        entry->playMinutes = item.value()[KEY_PLAY_MINUTES] | 0u;
        entry->lastPlayed = item.value()[KEY_LAST_PLAYED] | "";
        entry->path = item.value()[KEY_PATH] | "";
    }
    _sessionGameFileName = json[KEY_SESSION_GAME] | "";
    _sessionGameCode = json[KEY_SESSION_GAME_CODE] | "";
    _sessionStart = json[KEY_SESSION_START] | "";
}
