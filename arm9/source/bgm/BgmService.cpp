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
    if (!_audioStreamPlayer->StartPlayback(std::move(stream)))
    {
        _currentBgmName = "";
        return false;
    }

    SetDisplayNameFromPath(_currentBgmName, filePath);
    _bgmChangeId++;
    return true;
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
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(bgmFolder->GetFiles()[bgmToPlay]->GetFastFileRef()))
    {
        StopBgm();
        return;
    }

    if (!_audioStreamPlayer->StartPlayback(std::move(stream)))
    {
        StopBgm();
        return;
    }

    SetDisplayNameFromPath(_currentBgmName, bgmFolder->GetFiles()[bgmToPlay]->GetFileName());
    _bgmChangeId++;
}

void BgmService::StopBgm()
{
    _audioStreamPlayer->StopPlayback();
    _currentBgmName = "";
    _bgmChangeId++;
}

const char* BgmService::ConsumeBgmNameChange() const
{
    auto* self = const_cast<BgmService*>(this);
    if (self->_audioStreamPlayer && self->_audioStreamPlayer->ConsumePlaybackRestarted())
        self->_bgmChangeId++;
    if (self->_bgmChangeId == self->_bgmLastNotifiedId)
        return nullptr;

    self->_bgmLastNotifiedId = self->_bgmChangeId;
    return self->_currentBgmName.GetString();
}
