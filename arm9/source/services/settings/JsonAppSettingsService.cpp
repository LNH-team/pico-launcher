#include "common.h"
#include "JsonAppSettingsService.h"

JsonAppSettingsService::JsonAppSettingsService(const char* filePath)
    : _filePath(filePath)
{
    // Only write defaults when there's no settings file to lose - a file that exists but
    // failed to parse (corruption, a bad manual edit) must be left alone rather than wiped.
    if (_serializer.Deserialize(&_appSettings, _filePath) == DeserializeResult::NotFound)
        Save();
}
