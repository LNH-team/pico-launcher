#include "common.h"
#include <memory>
#include <cstring>
#include "fat/File.h"
#include "AppStateBin.h"
#include "AppStateBinSerializer.h"

#pragma GCC optimize("Os")

#define STATE_BIN_VERSION         1
#define STATE_BIN_THEME_NAME_SIZE 64

// Fixed-size header: version(1) + themeName(64) + rgb(3) + dark(1) + slot(4) + count(4) = 77
#define STATE_BIN_HEADER_SIZE (1 + STATE_BIN_THEME_NAME_SIZE + 1 + 1 + 1 + 1 + 4 + 4)

static const char* favStoredStr(const char* full)
{
    const char* colon = strchr(full, ':');
    return colon ? colon : full;
}

static u32 calcFavoritesSize(const AppStateBin* state)
{
    u32 total = 0;
    for (u32 i = 0; i < state->numberOfFavorites; i++)
    {
        const char* s = favStoredStr(state->favorites[i].GetString());
        u32 len = strlen(s);
        if (len > 255) len = 255;
        total += 2 + len;   
    }
    return total;
}

static u8* writeHeader(u8* p, const AppStateBin* state)
{
    *p++ = STATE_BIN_VERSION;

    const char* name = state->appliedThemeName.GetString();
    u32 nameLen = (u32)strlen(name);
    if (nameLen >= STATE_BIN_THEME_NAME_SIZE)
        nameLen = STATE_BIN_THEME_NAME_SIZE - 1;
    memcpy(p, name, nameLen);
    memset(p + nameLen, 0, STATE_BIN_THEME_NAME_SIZE - nameLen);
    p += STATE_BIN_THEME_NAME_SIZE;

    *p++ = state->primaryColorR;
    *p++ = state->primaryColorG;
    *p++ = state->primaryColorB;
    *p++ = state->darkTheme ? 1u : 0u;

    memcpy(p, &state->layoutSlot, 4);       p += 4;
    memcpy(p, &state->numberOfFavorites, 4); p += 4;

    return p;
}

static u8* writeFavorites(u8* p, const AppStateBin* state)
{
    for (u32 i = 0; i < state->numberOfFavorites; i++)
    {
        const char* s = favStoredStr(state->favorites[i].GetString());
        u16 len = (u16)strlen(s);
        if (len > 255) len = 255;
        memcpy(p, &len, 2); p += 2;
        memcpy(p, s, len);  p += len;
    }
    return p;
}

std::unique_ptr<u8[]> AppStateBinSerializer::SerializeToBuffer(
    const AppStateBin* state, u32& outLength) const
{
    const u32 totalSize = STATE_BIN_HEADER_SIZE + calcFavoritesSize(state);
    auto buf = std::make_unique<u8[]>(totalSize);
    u8* p = buf.get();
    p = writeHeader(p, state);
    writeFavorites(p, state);
    outLength = totalSize;
    return buf;
}

void AppStateBinSerializer::WriteBufferToFile(
    const u8* data, u32 length, const char* filePath) const
{
    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        return;
    }
    u32 bytesWritten = 0;
    file->Write(data, length, bytesWritten);
}

void AppStateBinSerializer::Serialize(const AppStateBin* state, const char* filePath) const
{
    u32 length = 0;
    auto buf = SerializeToBuffer(state, length);
    WriteBufferToFile(buf.get(), length, filePath);
}

bool AppStateBinSerializer::Deserialize(AppStateBin* state, const char* filePath) const
{
    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return false;

    const u32 fileSize = file->GetSize();
    if (fileSize < STATE_BIN_HEADER_SIZE)
        return false;

    auto buf = std::make_unique<u8[]>(fileSize);
    u32 bytesRead = 0;
    if (file->Read(buf.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
        return false;

    const u8* p   = buf.get();
    const u8* end = p + fileSize;

    if (*p++ != STATE_BIN_VERSION)
        return false;

    char themeName[STATE_BIN_THEME_NAME_SIZE + 1];
    memcpy(themeName, p, STATE_BIN_THEME_NAME_SIZE);
    themeName[STATE_BIN_THEME_NAME_SIZE] = '\0';
    state->appliedThemeName = themeName;
    p += STATE_BIN_THEME_NAME_SIZE;

    state->primaryColorR = *p++;
    state->primaryColorG = *p++;
    state->primaryColorB = *p++;
    state->darkTheme     = *p++;

    memcpy(&state->layoutSlot, p, 4); p += 4;
    if (state->layoutSlot < 1) state->layoutSlot = 1;

    u32 numberOfFavorites = 0;
    memcpy(&numberOfFavorites, p, 4); p += 4;

    if (numberOfFavorites > 0)
    {
        state->favorites = std::make_unique_for_overwrite<String<char, 256>[]>(numberOfFavorites);
        state->numberOfFavorites = 0;

        for (u32 i = 0; i < numberOfFavorites; i++)
        {
            if (p + 2 > end)
                break;
            u16 len = 0;
            memcpy(&len, p, 2); p += 2;
            if (p + len > end)
                break;
            if (len < 256)
            {
                char tmp[256];
                memcpy(tmp, p, len);
                tmp[len] = '\0';

                if (tmp[0] == ':')
                {
                    char full[256];
                    full[0] = 'f'; full[1] = 'a'; full[2] = 't';
                    u32 tlen = (u32)len;
                    if (tlen > 252) tlen = 252;
                    memcpy(full + 3, tmp, tlen);
                    full[3 + tlen] = '\0';
                    state->favorites[state->numberOfFavorites++] = full;
                }
                else if (strchr(tmp, ':') == nullptr)
                {
                    char full[256];
                    full[0] = 'f'; full[1] = 'a'; full[2] = 't'; full[3] = ':';
                    u32 tlen = (u32)len;
                    if (tlen > 251) tlen = 251;
                    memcpy(full + 4, tmp, tlen);
                    full[4 + tlen] = '\0';
                    state->favorites[state->numberOfFavorites++] = full;
                }
                else
                {
                    state->favorites[state->numberOfFavorites++] = tmp;
                }
            }
            p += len;
        }
    }

    return true;
}
