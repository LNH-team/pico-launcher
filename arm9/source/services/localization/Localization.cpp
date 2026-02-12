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
        if (!strcasecmp(key, "theme"))
            return u"Tema";
        if (!strcasecmp(key, "language"))
            return u"Lingua";
        if (!strcasecmp(key, "favorites"))
            return u"Preferiti";
        if (!strcasecmp(key, "cheats"))
            return u"Trucchi";
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
        if (!strcasecmp(key, "theme"))
            return u"Tema";
        if (!strcasecmp(key, "language"))
            return u"Idioma";
        if (!strcasecmp(key, "favorites"))
            return u"Favoritos";
        if (!strcasecmp(key, "cheats"))
            return u"Trucos";
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
        if (!strcasecmp(key, "theme"))
            return u"Thème";
        if (!strcasecmp(key, "language"))
            return u"Langue";
        if (!strcasecmp(key, "favorites"))
            return u"Favoris";
        if (!strcasecmp(key, "cheats"))
            return u"Triches";
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
        if (!strcasecmp(key, "theme"))
            return u"Thema";
        if (!strcasecmp(key, "language"))
            return u"Sprache";
        if (!strcasecmp(key, "favorites"))
            return u"Favoriten";
        if (!strcasecmp(key, "cheats"))
            return u"Codes";
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
        if (!strcasecmp(key, "theme"))
            return u"Tema";
        if (!strcasecmp(key, "language"))
            return u"Idioma";
        if (!strcasecmp(key, "favorites"))
            return u"Favoritos";
        if (!strcasecmp(key, "cheats"))
            return u"Truques";
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
        if (!strcasecmp(key, "theme"))
            return u"Thema";
        if (!strcasecmp(key, "language"))
            return u"Taal";
        if (!strcasecmp(key, "favorites"))
            return u"Favorieten";
        if (!strcasecmp(key, "cheats"))
            return u"Cheats";
    }

    // Default: English
    if (!strcasecmp(key, "display_settings"))
        return u"Display Settings";
    if (!strcasecmp(key, "layout"))
        return u"Layout";
    if (!strcasecmp(key, "sorting"))
        return u"Sorting";
    if (!strcasecmp(key, "theme"))
        return u"Theme";
    if (!strcasecmp(key, "language"))
        return u"Language";
    if (!strcasecmp(key, "favorites"))
        return u"Favorites";
    if (!strcasecmp(key, "cheats"))
        return u"Cheats";

    return u"";
}

