#pragma once
#include "services/settings/IAppSettingsService.h"

class Localization
{
public:
    static void Initialize(const IAppSettingsService* appSettingsService);
    static const char16_t* Translate(const char* key);
};
