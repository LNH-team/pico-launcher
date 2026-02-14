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

    /// @brief Starts playback of the background music for the given theme.
    virtual void StartBgmFromConfig(const char* themeName) = 0;

    /// @brief If currently playing, stops playback.
    virtual void StopBgm() = 0;

    /// @brief Gets the display name for the currently playing BGM.
    /// @return The display name, or an empty string if none.
    virtual const char* GetCurrentBgmName() const = 0;
};

inline IBgmService::~IBgmService() { }
