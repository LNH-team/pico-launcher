#pragma once
#include <memory>
#include "AppSettings.h"

class IAppSettingsService
{
public:
    virtual ~IAppSettingsService() { }

    virtual AppSettings& GetAppSettings() = 0;
    virtual const AppSettings& GetAppSettings() const = 0;
    virtual void Save() const = 0;

    /// @brief Serializes current settings to a buffer (safe to call from main thread).
    /// @param outLength Receives the length of the serialized data.
    /// @return The serialized data buffer.
    virtual std::unique_ptr<u8[]> SerializeToBuffer(u32& outLength) const = 0;

    /// @brief Writes a pre-serialized buffer to the settings file (safe to call from IO thread).
    /// @param data The serialized data.
    /// @param length The length of the data.
    virtual void WriteToFile(const u8* data, u32 length) const = 0;
};