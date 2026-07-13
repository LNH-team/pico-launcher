#include "common.h"
#include <memory>
#include <string.h>
#include <libtwl/rtos/rtosIrq.h>
#include "json/ArduinoJson.h"
#include "AppFavorites.h"
#include "fat/File.h"
#include "JsonFavoritesSerializer.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE  256

#define KEY_FAVORITES  "favorites"

// The live file is never written in place - see Serialize(). Both halves of that swap need
// the same temp name, derived from whatever path the service passes in.
#define FAVORITES_TEMP_SUFFIX  ".tmp"

static void buildTempPath(const char* filePath, char* tempPath, u32 tempPathSize)
{
    StringUtil::Copy(tempPath, filePath, tempPathSize);
    strlcat(tempPath, FAVORITES_TEMP_SUFFIX, tempPathSize);
}

// favorites can be replaced (and the old buffer freed) by a writer running on another
// thread at any time, so it's read the same way RomBrowserController::IsFavorite() does:
// never hold rtos_disableIrqs() across a heap allocation, only across the actual array
// reads. Since the count can change between sizing the snapshot and copying it, retry if
// it grew in between.
static std::unique_ptr<String<char, 256>[]> snapshotFavorites(const AppFavorites* appFavorites, u32& outCount)
{
    for (;;)
    {
        u32 capacity;
        {
            u32 irq = rtos_disableIrqs();
            capacity = appFavorites->numberOfFavorites;
            rtos_restoreIrqs(irq);
        }

        auto snapshot = std::make_unique_for_overwrite<String<char, 256>[]>(capacity);

        u32 irq = rtos_disableIrqs();
        outCount = appFavorites->numberOfFavorites;
        if (outCount > capacity)
        {
            rtos_restoreIrqs(irq);
            continue;
        }
        for (u32 i = 0; i < outCount; i++)
        {
            snapshot[i] = appFavorites->favorites[i];
        }
        rtos_restoreIrqs(irq);
        return snapshot;
    }
}

void JsonFavoritesSerializer::Serialize(const AppFavorites* appFavorites, const char* filePath) const
{
    u32 favoritesCount;
    std::unique_ptr<String<char, 256>[]> favoritesSnapshot = snapshotFavorites(appFavorites, favoritesCount);

    // favorites are added to the document by reference (JsonArray::add(const char*) links
    // rather than copies - see StringAdapter<const char*, void> in ArduinoJson.h), so only
    // the per-element structural slot needs budgeting here, not the string bytes themselves.
    size_t capacity = JSON_RESERVED_SIZE + JSON_ARRAY_SIZE(favoritesCount);
    DynamicJsonDocument json(capacity);

    JsonArray favoritesArray = json.createNestedArray(KEY_FAVORITES);
    for (u32 i = 0; i < favoritesCount; i++)
    {
        favoritesArray.add(favoritesSnapshot[i].GetString());
    }

    // ArduinoJson drops silently once its pool is exhausted (it has no error to return
    // here), so a pool that was sized short would serialize a partial list and Task 1's
    // rename would then swap that truncated list over the complete file on the card.
    if (json.overflowed())
    {
        LOG_ERROR("Favorites json pool exhausted, not saving\n");
        return;
    }

    u32 outputSize = measureJsonPretty(json);
    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);
    serializeJsonPretty(json, fileData.get(), outputSize);

    char tempPath[288];
    buildTempPath(filePath, tempPath, sizeof(tempPath));

    // Opening the live file with FA_CREATE_ALWAYS truncates it to zero before a single byte
    // is written, so losing power (or the card) mid-write leaves an empty file that the next
    // boot reads as "no favorites" and the next save makes permanent. Write a temp file and
    // only swap it in once it is complete, so the previous file survives until then.
    bool written = false;
    {
        const auto file = std::make_unique<File>();
        if (file->Open(tempPath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        {
            LOG_ERROR("Couldn't open favorites temp file for writing\n");
        }
        else
        {
            u32 bytesWritten;
            if (file->Write(fileData.get(), outputSize, bytesWritten) != FR_OK ||
                bytesWritten != outputSize)
            {
                LOG_ERROR("Error while writing favorites file\n");
            }
            // Close explicitly and check the result: the flush happens here, so a failure at
            // close means the temp file is incomplete. Letting ~File() close it silently
            // would rename a short file over the good one. (~File() calls f_close() again on
            // the already-closed handle, which returns FR_INVALID_OBJECT and does nothing.)
            else if (file->Close() != FR_OK)
            {
                LOG_ERROR("Error while flushing favorites file\n");
            }
            else
            {
                written = true;
            }
        }
    }

    if (!written)
    {
        f_unlink(tempPath);
        return;
    }

    // f_rename() refuses to overwrite an existing file, so the old one goes first. That
    // window is a few milliseconds wide and, unlike the truncate it replaces, a complete
    // replacement is already on the card while it is open - Deserialize() promotes it.
    f_unlink(filePath);
    if (f_rename(tempPath, filePath) != FR_OK)
    {
        LOG_ERROR("Couldn't swap in the new favorites file\n");
        return;
    }

    LOG_DEBUG("Favorites file written\n");
}

FavoritesDeserializeResult JsonFavoritesSerializer::Deserialize(AppFavorites* appFavorites, const char* filePath) const
{
    char tempPath[288];
    buildTempPath(filePath, tempPath, sizeof(tempPath));

    // A previous run may have died between the unlink and the rename in Serialize(). If it
    // did, this temp file is the only copy left and deleting it would destroy exactly what
    // the atomic save exists to protect, so promote it. Only when a live file is present is
    // the leftover a stale duplicate that is safe to drop.
    //
    // Promoting the temp file does not prove it is complete - the very first save a device
    // ever makes writes its temp file before any live file has ever existed, so losing power
    // during that write leaves exactly this situation (a temp file, no live file) with a
    // partial file behind it. Remember whether this call is the one that promoted the file,
    // so a promoted file that then fails to parse below can be told apart from a live file
    // that failed to parse: only the former is safe to discard.
    bool promotedTempFile = false;
    FILINFO tempFileInfo;
    if (f_stat(tempPath, &tempFileInfo) == FR_OK)
    {
        FILINFO liveFileInfo;
        if (f_stat(filePath, &liveFileInfo) == FR_OK)
        {
            f_unlink(tempPath);
        }
        else if (f_rename(tempPath, filePath) == FR_OK)
        {
            promotedTempFile = true;
            LOG_WARNING("Recovered favorites from an interrupted save\n");
        }
    }

    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        return FavoritesDeserializeResult::NotFound;
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
    {
        // An empty file is what an interrupted write left behind before Task 1's atomic
        // save, and it holds nothing to lose - so it is NotFound, not Error: defaults get
        // written over it and saving stays enabled, instead of latching saves off forever
        // on a file that is empty rather than corrupt.
        LOG_ERROR("Favorites file is empty\n");
        return FavoritesDeserializeResult::NotFound;
    }

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
    {
        return FavoritesDeserializeResult::Error;
    }

    // fileDataPtr is a mutable u8* (not const), so ArduinoJson already parses it in place
    // (zero-copy: it stores pointers into this buffer rather than duplicating strings), so
    // the pool only needs to cover structural overhead, not the file's string content.
    DynamicJsonDocument json(fileSize + JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
    {
        // A file this call just promoted from the temp name is, by definition, an incomplete
        // save with no complete file behind it - there is nothing left to protect by keeping
        // it around. Delete it and report NotFound so the caller writes fresh defaults and
        // saving keeps working, instead of latching off forever on a file that will never
        // parse. A file that was already live before this call is left alone: it may be the
        // only copy of favorites the user actually set, so Error is returned to leave it on
        // the card untouched.
        if (promotedTempFile)
        {
            f_unlink(filePath);
            return FavoritesDeserializeResult::NotFound;
        }
        return FavoritesDeserializeResult::Error;
    }

    JsonArrayConst favoritesArray = json[KEY_FAVORITES];
    if (!favoritesArray.isNull())
    {
        appFavorites->favorites = std::make_unique_for_overwrite<String<char, 256>[]>(favoritesArray.size());
        u32 i = 0;
        for (auto item : favoritesArray)
        {
            const char* path = item.as<const char*>();
            // String<char, 256> truncates silently on assignment, and a truncated path is a
            // prefix that can name a different file. A hand-edited file is the only way one
            // gets in here, so drop it rather than store a key nothing can match. The limit
            // is 255 characters, not the container's 256-character capacity: every buffer a
            // stored path has to match against elsewhere in the browser is a char[256], which
            // holds 255 usable characters plus the terminator, so a 256-character path could
            // never be matched even though it round-trips through this file untruncated.
            if (path == nullptr || strlen(path) > 255)
            {
                LOG_ERROR("Skipping favorites entry that is too long to store\n");
                continue;
            }
            appFavorites->favorites[i++] = path;
        }
        appFavorites->numberOfFavorites = i;
    }

    return FavoritesDeserializeResult::Success;
}
