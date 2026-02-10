#include "common.h"
#include "Localization.h"

static char s_languageBuf[32] = "english";

void Localization::Initialize(const IAppSettingsService* appSettingsService)
{
    if (!appSettingsService)
        return;

    auto& settings = appSettingsService->GetAppSettings();
    const char* lang = settings.language.GetString();
    if (!lang)
        return;

    // store lowercase language into local buffer
    size_t i = 0;
    for (; i < sizeof(s_languageBuf) - 1 && lang[i]; i++)
        s_languageBuf[i] = (char)tolower((unsigned char)lang[i]);
    s_languageBuf[i] = '\0';
}

const char16_t* Localization::Translate(const char* key)
{
    if (!key)
        return u"";

    // Italian
    if (!strcasecmp(s_languageBuf, "italian") || !strcasecmp(s_languageBuf, "italiano"))
    {
        if (!strcasecmp(key, "display_settings"))
            return u"Impostazioni";
        if (!strcasecmp(key, "layout"))
            return u"Disposizione";
        if (!strcasecmp(key, "sorting"))
            return u"Ordinamento";
    }

    // Spainish
    if (!strcasecmp(s_languageBuf, "spanish") || !strcasecmp(s_languageBuf, "espanol"))
    {
        if (!strcasecmp(key, "display_settings"))
            return u"Ajustes de pantalla";
        if (!strcasecmp(key, "layout"))
            return u"Diseño";
        if (!strcasecmp(key, "sorting"))
            return u"Clasificación";
    }

    // Default: English
    if (!strcasecmp(key, "display_settings"))
        return u"Display Settings";
    if (!strcasecmp(key, "layout"))
        return u"Layout";
    if (!strcasecmp(key, "sorting"))
        return u"Sorting";

    return u"";
}
