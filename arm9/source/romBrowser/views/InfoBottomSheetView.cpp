#include "common.h"
#include "core/Environment.h"
#include "core/StringUtil.h"
#include "core/mini-printf.h"
#include <nds/system.h>
#include "fat/Directory.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "picoLoaderBootstrap.h"
#include "services/localization/Localization.h"
#include "InfoBottomSheetView.h"

#define TITLE_LABEL_X       15
#define TITLE_LABEL_Y       20
#define INFO_LEFT_LABEL_X   16
#define INFO_RIGHT_LABEL_X  132
#define INFO_FIRST_LABEL_Y  36
#define INFO_LINE_SPACING   12
#define SWIPE_BACK_MIN_X    24
#define SWIPE_BACK_MAX_Y    28
#define SWIPE_BACK_START_X  96

static const char16_t* getModeValueText()
{
    PicoLoaderBootDrive bootDrive = (PicoLoaderBootDrive)(pload_getBootDrive() & ~PLOAD_BOOT_DRIVE_MULTIBOOT_FLAG);

    switch (bootDrive)
    {
        case PLOAD_BOOT_DRIVE_DSI_SD:
            return u"DSi SD";
        case PLOAD_BOOT_DRIVE_DLDI:
        default:
            return u"DLDI";
    }
}

static const char16_t* getConsoleValueText()
{
    if (Environment::IsDsiMode())
        return u"DSi / 3DS";

    return u"DS / DS Lite";
}

static const char16_t* getFirmwareColorText()
{
    static const char* colorKeys[16] =
    {
        "information_color_gray",
        "information_color_brown",
        "information_color_red",
        "information_color_pink",
        "information_color_orange",
        "information_color_yellow",
        "information_color_yellow_green",
        "information_color_green",
        "information_color_dark_green",
        "information_color_green_blue",
        "information_color_light_blue",
        "information_color_blue",
        "information_color_dark_blue",
        "information_color_dark_purple",
        "information_color_purple",
        "information_color_purple_red"
    };

    u8 color = PersonalData->theme;

    if (color < 16)
    {
        const char16_t* value = Localization::Translate(colorKeys[color]);
        if (value && value[0] != 0)
            return value;
    }

    return Localization::Translate("information_unknown");
}

static const char16_t* getFirmwareLanguageText()
{
    static const char* languageKeys[7] =
    {
        "information_language_japanese",
        "information_language_english",
        "information_language_french",
        "information_language_german",
        "information_language_italian",
        "information_language_spanish",
        "information_language_unknown"
    };

    u8 language = (u8)PersonalData->language;

    if (language < 7)
    {
        const char16_t* value = Localization::Translate(languageKeys[language]);
        if (value && value[0] != 0)
            return value;
    }

    return Localization::Translate("information_unknown");
}

static bool hasUsrcheatFile()
{
    FILINFO fileInfo;

    return f_stat("/_pico/extras/usrcheat.dat", &fileInfo) == FR_OK
        && (fileInfo.fattrib & AM_DIR) == 0;
}

static int countThemes()
{
    int count = 0;

    Directory directory;

    if (directory.Open("/_pico/themes") != FR_OK)
        return 0;

    FILINFO fileInfo;

    while (directory.Read(&fileInfo) == FR_OK)
    {
        if (fileInfo.fname[0] == 0)
            break;

        if (fileInfo.fname[0] == '.')
            continue;

        if ((fileInfo.fattrib & AM_DIR) == 0)
            continue;

        count++;
    }

    return count;
}

static int countLanguages()
{
    int count = 0;

    Directory directory;

    if (directory.Open("/_pico/extras/translations") != FR_OK)
        return 0;

    FILINFO fileInfo;

    while (directory.Read(&fileInfo) == FR_OK)
    {
        if (fileInfo.fname[0] == 0)
            break;

        if (fileInfo.fname[0] == '.')
            continue;

        if (fileInfo.fattrib & AM_DIR)
            continue;

        const char* dot = strrchr(fileInfo.fname, '.');

        if (!dot || strcasecmp(dot, ".bin") != 0)
            continue;

        count++;
    }

    return count;
}

static void copyAsciiToUtf16(char16_t* outText, u32 outTextLength, const char* text)
{
    if (!outText || outTextLength == 0)
        return;

    u32 i = 0;

    for (; i + 1 < outTextLength && text && text[i] != 0; i++)
        outText[i] = (char16_t)(unsigned char)text[i];

    outText[i] = 0;
}

static void copyUserNameFromFirmware(char16_t* outText, u32 outTextLength)
{
    if (!outText || outTextLength == 0)
        return;

    outText[0] = 0;

    u32 firmwareNameLen = PersonalData->nameLen;

    if (firmwareNameLen == 0 || firmwareNameLen > 10)
    {
        StringUtil::Copy(outText, u"Unknown", outTextLength);
        return;
    }

    u32 outIdx = 0;

    for (u32 i = 0; i < firmwareNameLen && outIdx + 1 < outTextLength; i++)
    {
        s16 ch = PersonalData->name[i];

        if (ch == 0)
            break;

        outText[outIdx++] = (char16_t)ch;
    }

    if (outIdx == 0)
    {
        StringUtil::Copy(outText, u"Unknown", outTextLength);
        return;
    }

    outText[outIdx] = 0;
}

static void copyUserMessageFromFirmware(char16_t* outText, u32 outTextLength)
{
    if (!outText || outTextLength == 0)
        return;

    outText[0] = 0;

    u32 firmwareMessageLen = PersonalData->messageLen;

    if (firmwareMessageLen > 26)
        firmwareMessageLen = 26;

    u32 outIdx = 0;

    for (u32 i = 0; i < firmwareMessageLen && outIdx + 1 < outTextLength; i++)
    {
        s16 ch = PersonalData->message[i];

        if (ch == 0)
            break;

        outText[outIdx++] = (char16_t)ch;
    }

    if (outIdx == 0)
    {
        outText[0] = 0;
        return;
    }

    outText[outIdx] = 0;
}

static void copyBirthDateFromFirmware(char16_t* outText, u32 outTextLength)
{
    if (!outText || outTextLength == 0)
        return;

    u8 day = PersonalData->birthDay;
    u8 month = PersonalData->birthMonth;

    if (day == 0 || day > 31 || month == 0 || month > 12)
    {
        StringUtil::Copy(outText, u"Unknown", outTextLength);
        return;
    }

    char dateBuf[16];
    mini_snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d", day, month);
    copyAsciiToUtf16(outText, outTextLength, dateBuf);
}

static void buildInfoLine(char16_t* outText, u32 outTextLength, const char16_t* label, const char16_t* value)
{
    if (!outText || outTextLength == 0)
        return;

    u32 idx = 0;

    if (label)
    {
        while (label[idx] != 0 && idx + 1 < outTextLength)
        {
            outText[idx] = label[idx];
            idx++;
        }
    }

    if (idx + 2 < outTextLength)
    {
        outText[idx++] = ':';
        outText[idx++] = ' ';
    }

    if (value)
    {
        for (u32 i = 0; value[i] != 0 && idx + 1 < outTextLength; i++)
            outText[idx++] = value[i];
    }

    outText[idx] = 0;
}

SettingsInfoBottomSheetView::SettingsInfoBottomSheetView(
    DisplaySettingsViewModel* viewModel,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _viewModel(viewModel)
    , _materialColorScheme(materialColorScheme)
    , _titleLabel(170, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _userLabel(116, 16, 64, fontRepository->GetFont(FontType::Regular10))
    , _birthdayLabel(116, 16, 64, fontRepository->GetFont(FontType::Regular10))
    , _messageLabel(228, 16, 96, fontRepository->GetFont(FontType::Regular10))
    , _colorLabel(228, 16, 96, fontRepository->GetFont(FontType::Regular10))
    , _consoleLanguageLabel(228, 16, 96, fontRepository->GetFont(FontType::Regular10))
    , _modeLabel(116, 16, 64, fontRepository->GetFont(FontType::Regular10))
    , _consoleLabel(116, 16, 64, fontRepository->GetFont(FontType::Regular10))
    , _usrcheatLabel(228, 16, 64, fontRepository->GetFont(FontType::Regular10))
    , _themesLabel(116, 16, 64, fontRepository->GetFont(FontType::Regular10))
    , _languagesLabel(116, 16, 64, fontRepository->GetFont(FontType::Regular10))
{
    _titleLabel.SetText(Localization::Translate("information"));

    char16_t userName[24];
    copyUserNameFromFirmware(userName, 24);

    char16_t birthDate[24];
    copyBirthDateFromFirmware(birthDate, 24);

    char16_t userMessage[32];
    copyUserMessageFromFirmware(userMessage, 32);

    char16_t userLine[72];
    char16_t birthdayLine[72];
    char16_t messageLine[96];
    char16_t colorLine[128];
    char16_t consoleLanguageLine[128];
    char16_t modeLine[72];
    char16_t consoleLine[72];
    char16_t usrcheatLine[96];
    char16_t themesLine[72];
    char16_t languagesLine[72];

    buildInfoLine(userLine, sizeof(userLine) / sizeof(userLine[0]),
        Localization::Translate("information_user"), userName);

    buildInfoLine(birthdayLine, sizeof(birthdayLine) / sizeof(birthdayLine[0]),
        Localization::Translate("information_birthdate"), birthDate);

    buildInfoLine(messageLine, sizeof(messageLine) / sizeof(messageLine[0]),
        Localization::Translate("information_message"), userMessage);

    buildInfoLine(colorLine, sizeof(colorLine) / sizeof(colorLine[0]),
        Localization::Translate("information_favorite_color"), getFirmwareColorText());

    buildInfoLine(consoleLanguageLine, sizeof(consoleLanguageLine) / sizeof(consoleLanguageLine[0]),
        Localization::Translate("information_console_language"), getFirmwareLanguageText());

    buildInfoLine(modeLine, sizeof(modeLine) / sizeof(modeLine[0]),
        Localization::Translate("information_mode"), getModeValueText());

    buildInfoLine(consoleLine, sizeof(consoleLine) / sizeof(consoleLine[0]),
        Localization::Translate("information_console"), getConsoleValueText());
        
        buildInfoLine(usrcheatLine, sizeof(usrcheatLine) / sizeof(usrcheatLine[0]),
        u"usrcheat.dat",
        hasUsrcheatFile() ? Localization::Translate("information_usrcheat_found") : Localization::Translate("information_usrcheat_not_found"));

    char themesCount[16];
    mini_snprintf(themesCount, sizeof(themesCount), "%d", countThemes());

    char16_t themesCount16[16];
    copyAsciiToUtf16(themesCount16, 16, themesCount);

    buildInfoLine(themesLine, sizeof(themesLine) / sizeof(themesLine[0]),
        Localization::Translate("information_themes"), themesCount16);

    char languagesCount[16];
    mini_snprintf(languagesCount, sizeof(languagesCount), "%d", countLanguages());

    char16_t languagesCount16[16];
    copyAsciiToUtf16(languagesCount16, 16, languagesCount);

    buildInfoLine(languagesLine, sizeof(languagesLine) / sizeof(languagesLine[0]),
        Localization::Translate("information_languages"), languagesCount16);

    _userLabel.SetText(userLine);
    _birthdayLabel.SetText(birthdayLine);
    _colorLabel.SetText(colorLine);
    _consoleLanguageLabel.SetText(consoleLanguageLine);
    _messageLabel.SetText(messageLine);
    _consoleLabel.SetText(consoleLine);
    _modeLabel.SetText(modeLine);
    _usrcheatLabel.SetText(usrcheatLine);
    _themesLabel.SetText(themesLine);
    _languagesLabel.SetText(languagesLine);

    AddChildTail(&_titleLabel);
    AddChildTail(&_userLabel);
    AddChildTail(&_birthdayLabel);
    AddChildTail(&_colorLabel);
    AddChildTail(&_consoleLanguageLabel);
    AddChildTail(&_messageLabel);
    AddChildTail(&_consoleLabel);
    AddChildTail(&_modeLabel);
    AddChildTail(&_usrcheatLabel);
    AddChildTail(&_themesLabel);
    AddChildTail(&_languagesLabel);
}

void SettingsInfoBottomSheetView::Update()
{
    BottomSheetView::Update();

    _titleLabel.SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    int lineY = _position.y + INFO_FIRST_LABEL_Y;

    _userLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    _birthdayLabel.SetPosition(INFO_RIGHT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING;
    _colorLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING;
    _consoleLanguageLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING;
    _messageLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING;
    _consoleLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    _modeLabel.SetPosition(INFO_RIGHT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING;
    _usrcheatLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING;
    _themesLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    _languagesLabel.SetPosition(INFO_RIGHT_LABEL_X, lineY);
}

void SettingsInfoBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());

    u32 oldPrio = graphicsContext.SetPriority(1);

    {
        auto bgColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        auto fgColor = _materialColorScheme->onSurface;
        auto fgVariant = _materialColorScheme->onSurfaceVariant;

        _titleLabel.SetBackgroundColor(bgColor);
        _titleLabel.SetForegroundColor(fgColor);

        _userLabel.SetBackgroundColor(bgColor);
        _userLabel.SetForegroundColor(fgVariant);

        _birthdayLabel.SetBackgroundColor(bgColor);
        _birthdayLabel.SetForegroundColor(fgVariant);

        _messageLabel.SetBackgroundColor(bgColor);
        _messageLabel.SetForegroundColor(fgVariant);

        _colorLabel.SetBackgroundColor(bgColor);
        _colorLabel.SetForegroundColor(fgVariant);

        _consoleLanguageLabel.SetBackgroundColor(bgColor);
        _consoleLanguageLabel.SetForegroundColor(fgColor);

        _modeLabel.SetBackgroundColor(bgColor);
        _modeLabel.SetForegroundColor(fgVariant);

        _consoleLabel.SetBackgroundColor(bgColor);
        _consoleLabel.SetForegroundColor(fgVariant);

        _usrcheatLabel.SetBackgroundColor(bgColor);
        _usrcheatLabel.SetForegroundColor(fgVariant);

        _themesLabel.SetBackgroundColor(bgColor);
        _themesLabel.SetForegroundColor(fgVariant);

        _languagesLabel.SetBackgroundColor(bgColor);
        _languagesLabel.SetForegroundColor(fgVariant);

        BottomSheetView::Draw(graphicsContext);
    }

    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool SettingsInfoBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    (void)focusManager;

    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->HideInfo();
        return true;
    }

    if (inputProvider.Triggered(InputKey::L))
    {
        _viewModel->HideInfo();
        return true;
    }

    return false;
}

bool SettingsInfoBottomSheetView::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    (void)focusManager;

    if (event.type != TouchEventType::Up)
        return false;

    int deltaX = event.position.x - event.startPosition.x;
    int deltaY = event.position.y - event.startPosition.y;
    int absDeltaY = deltaY < 0 ? -deltaY : deltaY;

    if (event.startPosition.x <= SWIPE_BACK_START_X
        && deltaX >= SWIPE_BACK_MIN_X
        && absDeltaY <= SWIPE_BACK_MAX_Y)
    {
        _viewModel->HideInfo();
        return true;
    }

    return false;
}

void SettingsInfoBottomSheetView::OnDismissed()
{
    _viewModel->HideInfo();
}