#include "common.h"
#include "core/mini-printf.h"
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
        return false;
        
    return _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::StartBgmFromConfig(const std::string& _effectiveThemeName)
{
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/bgm", _effectiveThemeName.c_str());
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

    _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::StopBgm()
{
    _audioStreamPlayer->StopPlayback();
}
