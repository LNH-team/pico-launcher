#include "common.h"
#include "core/Environment.h"
#include "core/StringUtil.h"
#include "core/mini-printf.h"
#include <nds/system.h>
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "picoLoaderBootstrap.h"
#include "services/Localization/Localization.h"
#include "../IRomBrowserController.h"
#include "InfoBottomSheetView.h"

#define TITLE_LABEL_X       15
#define TITLE_LABEL_Y       8
#define INFO_LEFT_LABEL_X   16
#define INFO_RIGHT_LABEL_X  132
#define INFO_FIRST_LABEL_Y  30
#define INFO_LINE_SPACING   12
#define INFO_GAP 8

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
    static bool s_cached = false;
    static bool s_loaded = false;

    if (s_loaded)
        return s_cached;

    FILINFO fileInfo;
    s_cached = f_stat("/_pico/extras/usrcheat.dat", &fileInfo) == FR_OK
        && (fileInfo.fattrib & AM_DIR) == 0;
    s_loaded = true;

    return s_cached;
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
        StringUtil::Copy(outText, Localization::Translate("information_unknown"), outTextLength);
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
        StringUtil::Copy(outText, Localization::Translate("information_unknown"), outTextLength);
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
        StringUtil::Copy(outText, Localization::Translate("information_unknown"), outTextLength);
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
        StringUtil::Copy(outText, Localization::Translate("information_unknown"), outTextLength);
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

static void buildTouchLine(char16_t* outText, u32 outTextLength, int x, int y)
{
    char coords[24];
    mini_snprintf(coords, sizeof(coords), "X:%d      Y:%d", x, y);
    
    char16_t coords16[24];
    copyAsciiToUtf16(coords16, sizeof(coords16) / sizeof(coords16[0]), coords);

    buildInfoLine(outText, outTextLength, u"Touch", coords16);
}

void SettingsInfoBottomSheetView::UpdateTouchLabelText(int x, int y)
{
    if (_displayedTouchX == x && _displayedTouchY == y)
        return;

    char16_t touchLine[72];
    buildTouchLine(touchLine, sizeof(touchLine) / sizeof(touchLine[0]), x, y);
    _touchLabel.SetText(touchLine);
    _displayedTouchX = x;
    _displayedTouchY = y;
}

SettingsInfoBottomSheetView::SettingsInfoBottomSheetView(
    IRomBrowserController* romBrowserController,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _romBrowserController(romBrowserController)
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
    , _touchLabel(300, 16, 64, fontRepository->GetFont(FontType::Regular10))
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

    _userLabel.SetText(userLine);
    _birthdayLabel.SetText(birthdayLine);
    _colorLabel.SetText(colorLine);
    _consoleLanguageLabel.SetText(consoleLanguageLine);
    _messageLabel.SetText(messageLine);
    _consoleLabel.SetText(consoleLine);
    _modeLabel.SetText(modeLine);
    _usrcheatLabel.SetText(usrcheatLine);
    UpdateTouchLabelText(0, 0);

    AddChildTail(&_titleLabel);
    AddChildTail(&_userLabel);
    AddChildTail(&_birthdayLabel);
    AddChildTail(&_colorLabel);
    AddChildTail(&_consoleLanguageLabel);
    AddChildTail(&_messageLabel);
    AddChildTail(&_consoleLabel);
    AddChildTail(&_modeLabel);
    AddChildTail(&_usrcheatLabel);
    AddChildTail(&_touchLabel);
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
    lineY += INFO_LINE_SPACING + INFO_GAP;
    _consoleLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    _modeLabel.SetPosition(INFO_RIGHT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING + INFO_GAP;
    _usrcheatLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
    lineY += INFO_LINE_SPACING + INFO_GAP;
    _touchLabel.SetPosition(INFO_LEFT_LABEL_X, lineY);
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

        _touchLabel.SetBackgroundColor(bgColor);
        _touchLabel.SetForegroundColor(fgVariant);

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
        _romBrowserController->HideDisplayInfo();
        return true;
    }

    if (inputProvider.Triggered(InputKey::L))
    {
        _romBrowserController->HideDisplayInfo();
        return true;
    }

    return false;
}

bool SettingsInfoBottomSheetView::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    (void)focusManager;

    if (event.type == TouchEventType::Down || event.type == TouchEventType::Move)
    {
        _touchX = event.position.x;
        _touchY = event.position.y;
        // Avoid rewriting the label every frame when the coordinates are unchanged.
        UpdateTouchLabelText(_touchX, _touchY);
        return false;
    }

    if (event.type == TouchEventType::Up)
    {
        _touchX = 0;
        _touchY = 0;
        UpdateTouchLabelText(0, 0);
    }
    else
    {
        return false;
    }

    return false;
}

void SettingsInfoBottomSheetView::OnDismissed()
{
    _romBrowserController->HideDisplayInfo();
}
