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
            return u"Impostazioni schermo";
        if (!strcasecmp(key, "layout"))
            return u"Disposizione";
        if (!strcasecmp(key, "sorting"))
            return u"Ordinamento";
    }

    // Spanish
    if (!strcasecmp(s_languageBuf, "spanish") || !strcasecmp(s_languageBuf, "espanol"))
    {
        if (!strcasecmp(key, "display_settings"))
            return u"Ajustes de pantalla";
        if (!strcasecmp(key, "layout"))
            return u"Diseño";
        if (!strcasecmp(key, "sorting"))
            return u"Clasificación";
    }

    // French
    if (!strcasecmp(s_languageBuf, "french") || !strcasecmp(s_languageBuf, "francais"))
    {
        if (!strcasecmp(key, "display_settings"))
            return u"Paramètres d’affichage";
        if (!strcasecmp(key, "layout"))
            return u"Disposition";
        if (!strcasecmp(key, "sorting"))
            return u"Tri";
    }

    // German
    if (!strcasecmp(s_languageBuf, "german") || !strcasecmp(s_languageBuf, "deutsch"))
    {
        if (!strcasecmp(key, "display_settings"))
            return u"Anzeigeeinstellungen";
        if (!strcasecmp(key, "layout"))
            return u"Layout";
        if (!strcasecmp(key, "sorting"))
            return u"Sortierung";
    }

    // Portuguese
    if (!strcasecmp(s_languageBuf, "portuguese") || !strcasecmp(s_languageBuf, "portugues"))
    {
        if (!strcasecmp(key, "display_settings"))
            return u"Configurações de exibição";
        if (!strcasecmp(key, "layout"))
            return u"Layout";
        if (!strcasecmp(key, "sorting"))
            return u"Ordenação";
    }

    // Dutch
    if (!strcasecmp(s_languageBuf, "dutch") || !strcasecmp(s_languageBuf, "nederlands"))
    {
        if (!strcasecmp(key, "display_settings"))
            return u"Weergave-instellingen";
        if (!strcasecmp(key, "layout"))
            return u"Indeling";
        if (!strcasecmp(key, "sorting"))
            return u"Sortering";
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

