#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "Pcm16FileAudioStream.h"
#include "BcstmAudioStream.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/SdFolderFactory.h"
#include "romBrowser/FileType/NullFileTypeProvider.h"
#include "BgmService.h"

namespace
{
    void SetDisplayNameFromPath(String<char, 64>& target, const TCHAR* filePath)
    {
        if (!filePath || filePath[0] == 0)
        {
            target = "";
            return;
        }

        const char* nameStart = strrchr(filePath, '/');
        nameStart = nameStart ? nameStart + 1 : filePath;

        char nameBuffer[64];
        StringUtil::Copy(nameBuffer, nameStart, sizeof(nameBuffer));
        char* dot = strrchr(nameBuffer, '.');
        if (dot && dot != nameBuffer)
            *dot = 0;

        target = nameBuffer;
    }
}

bool BgmService::StartBgm(const TCHAR* filePath)
{
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(filePath))
    {
        _currentBgmName = "";
        return false;
    }
    SetDisplayNameFromPath(_currentBgmName, filePath);
    return _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::StartBgmFromConfig(const char* themeName)
{
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/bgm", themeName);
    NullFileTypeProvider fileTypeProvider;
    auto bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath(pathBuffer);
    if (!bgmFolder || bgmFolder->GetFileCount() == 0)
    {
        StopBgm();
        return;
    }
    u32 bgmToPlay = _randomGenerator.NextU32(bgmFolder->GetFileCount());
    SetDisplayNameFromPath(_currentBgmName, bgmFolder->GetFiles()[bgmToPlay]->GetFileName());
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(bgmFolder->GetFiles()[bgmToPlay]->GetFastFileRef()))
    {
        StopBgm();
        return;
    }

    _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::StopBgm()
{
    _audioStreamPlayer->StopPlayback();
    _currentBgmName = "";
}
