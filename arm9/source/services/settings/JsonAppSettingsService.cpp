#include "common.h"
#include "JsonAppSettingsService.h"
#include "translate.h"

JsonAppSettingsService::JsonAppSettingsService(const char* filePath)
    : _filePath(filePath)
{
    if (!_serializer.Deserialize(&_appSettings, _filePath))
        Save();
    
    SetLanguage(_appSettings.language.GetString());
}
