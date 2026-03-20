#pragma once
#include "IAppSettingsService.h"
#include "JsonAppSettingsSerializer.h"

class JsonAppSettingsService : public IAppSettingsService
{
    JsonAppSettingsSerializer _serializer;
    AppSettings _appSettings;
    const char* _filePath;
public:
    explicit JsonAppSettingsService(const char* filePath);

    AppSettings& GetAppSettings() override { return _appSettings; }
    const AppSettings& GetAppSettings() const override { return _appSettings; }

    void Save() const override
    {
        _serializer.Serialize(&_appSettings, _filePath);
    }

    std::unique_ptr<u8[]> SerializeToBuffer(u32& outLength) const override
    {
        return _serializer.SerializeToBuffer(&_appSettings, outLength);
    }

    void WriteToFile(const u8* data, u32 length) const override
    {
        _serializer.WriteBufferToFile(data, length, _filePath);
    }
};