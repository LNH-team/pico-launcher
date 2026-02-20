#pragma once
#include "fat/ff.h"

/// @brief Interface for a background music service.
class IBgmService
{
public:
    virtual ~IBgmService() = 0;

    /// @brief Starts playback of the given file.
    /// @param filePath The file to play.
    /// @return True if playback was successfully started, or false otherwise.
    virtual bool StartBgm(const TCHAR* filePath) = 0;

    /// @brief Starts playback of the background music according to the app config.
    /// @param _effectiveThemeName The name of the theme to use for background music.
    virtual void StartBgmFromConfig(const std::string& _effectiveThemeName) = 0;

    /// @brief If currently playing, stops playback.
    virtual void StopBgm() = 0;
};

inline IBgmService::~IBgmService() { }
