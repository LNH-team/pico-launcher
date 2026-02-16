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

bool BgmService::StartBgm(const TCHAR* filePath)
{
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(filePath))
    {
        return false;
    }
    if (!_audioStreamPlayer->StartPlayback(std::move(stream)))
    {
        return false;
    }

    // ...existing code...
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

    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(bgmFolder->GetFiles()[0]->GetFastFileRef()))
    {
        StopBgm();
        return;
    }

    if (!_audioStreamPlayer->StartPlayback(std::move(stream)))
    {
        StopBgm();
        return;
    }
    
    // ...existing code...
}

void BgmService::StopBgm()
{
    _audioStreamPlayer->StopPlayback();
}


