#pragma once
class AppSettings;

enum class DeserializeResult
{
    Success,
    NotFound,
    Error
};

class JsonAppSettingsSerializer
{
public:
    void Serialize(const AppSettings* appSettings, const char* filePath) const;
    DeserializeResult Deserialize(AppSettings* appSettings, const char* filePath) const;
};