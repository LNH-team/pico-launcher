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
    const auto& settings = _appSettingsService.GetAppSettings();
    const char* bgmSetting = settings.bgm.GetString();

    // Load BGM file list from /_pico/bgm/
    NullFileTypeProvider fileTypeProvider;
    auto bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/bgm");
    if (!bgmFolder || bgmFolder->GetFileCount() == 0)
    {
        // Fallback: try the theme-specific bgm directory
        TCHAR pathBuffer[128];
        mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/bgm", _effectiveThemeName.c_str());
        bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath(pathBuffer);
        if (!bgmFolder || bgmFolder->GetFileCount() == 0)
        {
            StopBgm();
            return;
        }
    }

    // If a specific BGM is configured, try to find and play it
    if (bgmSetting && bgmSetting[0] != '\0')
    {
        for (int i = 0; i < bgmFolder->GetFileCount(); i++)
        {
            const auto* file = bgmFolder->GetFiles()[i];
            if (!strcasecmp(file->GetFileName(), bgmSetting))
            {
                auto stream = std::make_unique<BcstmAudioStream>();
                if (stream->Open(file->GetFastFileRef()))
                {
                    _audioStreamPlayer->StartPlayback(std::move(stream));
                    return;
                }
                break;
            }
        }
        // If specified file not found, fall through to random
    }

    // Random playback
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
