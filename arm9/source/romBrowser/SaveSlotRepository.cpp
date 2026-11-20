#include "common.h"
#include <string.h>
#include <strings.h>
#include "fat/File.h"
#include "SaveSlotRepository.h"

#define MAX_FILE_SIZE   (64 * 1024)

void SaveSlotRepository::Load(const char* filePath)
{
    _entryCount = 0;
    _dirty = false;

    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_READ) != FR_OK)
    {
        LOG_DEBUG("No save slot file found\n");
        return;
    }

    u32 size = file->GetSize();
    if (size == 0 || size > MAX_FILE_SIZE)
    {
        return;
    }

    std::unique_ptr<char[]> data(new(cache_align) char[size + 1]);
    if (!file->ReadExact(data.get(), size))
    {
        LOG_ERROR("Failed to read save slot file\n");
        return;
    }
    data[size] = 0;

    char* line = data.get();
    while (*line != 0)
    {
        char* lineEnd = strchr(line, '\n');
        char* next = lineEnd ? lineEnd + 1 : line + strlen(line);
        if (lineEnd)
        {
            *lineEnd = 0;
        }

        u32 lineLength = strlen(line);
        if (lineLength > 0 && line[lineLength - 1] == '\r')
        {
            line[--lineLength] = 0;
        }

        // Format: "<slot>\t<rom path>"; the rom path includes the volume id (e.g. "fat:/roms/game.nds")
        if (lineLength >= 3 && line[0] >= '1' && line[0] <= '0' + (char)MaxSlot && line[1] == '\t')
        {
            if (FindEntry(&line[2]) < 0)
            {
                AddEntry(&line[2], lineLength - 2, line[0] - '0');
            }
        }

        line = next;
    }

    LOG_DEBUG("Loaded %d save slot selections\n", _entryCount);
}

u32 SaveSlotRepository::GetSlot(const char* romPath) const
{
    int index = FindEntry(romPath);
    return index < 0 ? 0 : _entries[index].slot;
}

void SaveSlotRepository::SetSlot(const char* romPath, u32 slot)
{
    if (slot > MaxSlot)
    {
        return;
    }

    int index = FindEntry(romPath);
    if (index >= 0)
    {
        if (_entries[index].slot == slot)
        {
            return;
        }

        if (slot == 0)
        {
            // Back to the default save; remove the entry
            _entries[index] = std::move(_entries[_entryCount - 1]);
            _entryCount--;
        }
        else
        {
            _entries[index].slot = slot;
        }
        _dirty = true;
    }
    else if (slot != 0)
    {
        u32 oldCount = _entryCount;
        AddEntry(romPath, strlen(romPath), slot);
        _dirty = _entryCount != oldCount;
    }
}

std::unique_ptr<char[]> SaveSlotRepository::Serialize(u32& length)
{
    length = 0;
    for (u32 i = 0; i < _entryCount; i++)
    {
        length += 2 + strlen(_entries[i].romPath.get()) + 1;
    }

    std::unique_ptr<char[]> data(new(cache_align) char[length + 1]);
    char* ptr = data.get();
    for (u32 i = 0; i < _entryCount; i++)
    {
        *ptr++ = '0' + _entries[i].slot;
        *ptr++ = '\t';
        u32 pathLength = strlen(_entries[i].romPath.get());
        memcpy(ptr, _entries[i].romPath.get(), pathLength);
        ptr += pathLength;
        *ptr++ = '\n';
    }
    *ptr = 0;

    _dirty = false;
    return data;
}

void SaveSlotRepository::WriteFile(const char* filePath, const char* data, u32 length)
{
    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open save slot file for writing\n");
        return;
    }

    u32 bytesWritten = 0;
    if (length != 0 && (file->Write(data, length, bytesWritten) != FR_OK || bytesWritten != length))
    {
        LOG_ERROR("Error while writing save slot file\n");
    }
}

bool SaveSlotRepository::TryCreateSavePath(const char* romPath, u32 slot, char* savePath, u32 savePathSize)
{
    if (slot == 0)
    {
        // Leave the save path empty so Pico Loader uses its default
        return false;
    }

    return CreateSavePath(romPath, slot, savePath, savePathSize);
}

u16 SaveSlotRepository::GetExistingSavesMask(const char* romPath)
{
    u16 mask = 0;
    auto fileInfo = std::make_unique<FILINFO>();
    char savePath[256];
    for (u32 slot = 0; slot <= MaxSlot; slot++)
    {
        if (CreateSavePath(romPath, slot, savePath, sizeof(savePath))
            && f_stat(savePath, fileInfo.get()) == FR_OK
            && !(fileInfo->fattrib & AM_DIR))
        {
            mask |= 1 << slot;
        }
    }
    return mask;
}

bool SaveSlotRepository::CreateSavePath(const char* romPath, u32 slot, char* savePath, u32 savePathSize)
{
    if (slot > MaxSlot)
    {
        return false;
    }

    // Same logic as Pico Loader: replace the extension of the rom path, or append when there is none
    const char* lastSlash = strrchr(romPath, '/');
    const char* extension = strrchr(romPath, '.');
    if (!extension || (lastSlash && extension < lastSlash))
    {
        extension = romPath + strlen(romPath);
    }

    u32 baseLength = extension - romPath;
    if (baseLength + sizeof(".0.sav") > savePathSize)
    {
        return false;
    }

    // Slot 0 is rom.sav, slot N is rom.N.sav
    memcpy(savePath, romPath, baseLength);
    char* ptr = savePath + baseLength;
    if (slot != 0)
    {
        *ptr++ = '.';
        *ptr++ = '0' + slot;
    }
    memcpy(ptr, ".sav", sizeof(".sav"));
    return true;
}

int SaveSlotRepository::FindEntry(const char* romPath) const
{
    for (u32 i = 0; i < _entryCount; i++)
    {
        if (strcasecmp(_entries[i].romPath.get(), romPath) == 0)
        {
            return i;
        }
    }
    return -1;
}

void SaveSlotRepository::AddEntry(const char* romPath, u32 romPathLength, u32 slot)
{
    if (_entryCount >= MaxEntries)
    {
        LOG_ERROR("Too many save slot entries\n");
        return;
    }

    auto path = std::make_unique<char[]>(romPathLength + 1);
    memcpy(path.get(), romPath, romPathLength);
    path[romPathLength] = 0;
    _entries[_entryCount].romPath = std::move(path);
    _entries[_entryCount].slot = slot;
    _entryCount++;
}
