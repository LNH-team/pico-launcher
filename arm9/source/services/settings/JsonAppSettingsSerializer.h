#pragma once
#include <memory>

class AppSettings;

class JsonAppSettingsSerializer
{
public:
    void Serialize(const AppSettings* appSettings, const char* filePath) const;
    bool Deserialize(AppSettings* appSettings, const char* filePath) const;

    std::unique_ptr<u8[]> SerializeToBuffer(const AppSettings* appSettings, u32& outLength) const;
    void WriteBufferToFile(const u8* data, u32 length, const char* filePath) const;
};