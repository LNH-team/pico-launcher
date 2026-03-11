#include "common.h"
#include <algorithm>
#include "LayoutEditorBottomSheetView.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/FocusManager.h"
#include "gui/materialDesign.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/material/MaterialColorSchemeFactory.h"
#include "themes/IFontRepository.h"
#include "themes/FontType.h"
#include "core/mini-printf.h"
#include "fat/File.h"
#include "json/ArduinoJson.h"
#include "services/settings/IAppSettingsService.h"
#include "../IRomBrowserController.h"
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfxPalette.h>
#include "core/math/RgbMixer.h"
#include "core/math/ColorConverter.h"

// Y positions
#define LE_TITLE_Y            8
#define LE_SLOT_Y             30
#define LE_SAVERES_Y          30
#define LE_SUBMENU_Y          48
#define LE_ITEM_Y0            62
#define LE_ITEM_Y0_SUBMENU    30
#define LE_ITEM_YSTEP         14
#define LE_HINT_Y             186

// X positions
#define LE_TITLE_X      15
#define LE_SLOT_X       15
#define LE_SAVE_X       72
#define LE_RESET_X      124
#define LE_RESET_MENU_X 172
#define LE_SUBMENU_X    15
#define LE_ITEMNAME_X   20
#define LE_ITEMVALUE_X  120
#define LE_HINT_X       15

#define LE_FOCUS_OFFSET_X 6

// submenus.
#define UI_SUBMENU_DATETIME1  0
#define UI_SUBMENU_DATETIME2  1
#define UI_SUBMENU_PREFIX     2
#define UI_SUBMENU_GAME_ID    3
#define UI_SUBMENU_REGION     4
#define UI_SUBMENU_VERSION    5
#define UI_SUBMENU_CRC        6
#define UI_SUBMENU_USERNAME   7
#define UI_SUBMENU_BOXART     8
#define UI_SUBMENU_ICON       9
#define UI_SUBMENU_ROMNAME    10
#define UI_SUBMENU_FILENAME   11
#define UI_SUBMENU_THEMECOLOR 12
#define UI_SUBMENU_COUNT      13

static const char* const kUiSubMenuNames[UI_SUBMENU_COUNT] = {
    "DateTime1",
    "DateTime2",
    "PREFIX",
    "GAME ID",
    "Region",
    "Version",
    "CRC",
    "Username",
    "Box Art",
    "Icon",
    "ROM Name",
    "File Name",
    "Theme Color",
};

static const char* const kItemNamesDateTime[] = {
    "Visible", "Y", "X", "Format", "Separator", "Font", "Color R", "Color G", "Color B"
};
static const char* const kItemNamesPrefix[] = {
    "Visible", "X", "Y", "Font", "Trailing -", "GBA Text", "NTR Text", "TWL Text", "Color R", "Color G", "Color B"
};
static const char* const kItemNamesGameId[] = {
    "Visible", "X", "Y", "Font", "Trailing -", "Show TID", "TID Font", "TID X", "TID Y", "TID Color R", "TID Color G", "TID Color B", "Color R", "Color G", "Color B"
};
static const char* const kItemNamesRegion[] = {
    "Visible", "X", "Y", "Font", "Trailing -", "Color R", "Color G", "Color B"
};
static const char* const kItemNamesElement[] = {
    "Visible", "Y", "X", "Font", "Color R", "Color G", "Color B"
};
static const char* const kItemNamesNoFont[] = {
    "Visible", "Y", "X"
};
static const char* const kItemNamesFileName[] = {
    "Visible", "Y", "X", "Font", "Scroll", "Speed", "Color R", "Color G", "Color B"
};
static const char* const kItemNamesRomName[] = {
    "Line", "Visible", "X", "Y", "Font", "Color R", "Color G", "Color B"
};
static const char* const kItemNamesThemeColor[] = {
    "R", "G", "B", "Dark", "Preview"
};

static const u8 kSubMenuItemCounts[UI_SUBMENU_COUNT] = {
    9,
    9,
    11,
    15,
    8,
    7,
    7,
    7,
    3,
    3,
    8,
    9,
    5,
};

#define THEME_COLOR_JSON_RESERVED_SIZE 3072
#define THEME_COLOR_KEY_PRIMARY_COLOR "primaryColor"
#define THEME_COLOR_KEY_DARK_THEME "darkTheme"
#define THEME_COLOR_KEY_R "r"
#define THEME_COLOR_KEY_G "g"
#define THEME_COLOR_KEY_B "b"

static void ChangeWrappedCoord(s16& v, int delta, int minValue, int maxValue)
{
    int n = (int)v + delta;
    if (n > maxValue)
        n = minValue;
    else if (n < minValue)
        n = maxValue;
    v = (s16)n;
}

static int GetHoldAccelerationStep(u16 holdFrames)
{
    if (holdFrames < 18)
        return 1;
    if (holdFrames < 42)
        return 2;
    if (holdFrames < 72)
        return 3;
    return 5;
}

static int WrapRange(int value, int minValue, int maxValue)
{
    if (maxValue < minValue)
        return minValue;
    const int range = maxValue - minValue + 1;
    int n = (value - minValue) % range;
    if (n < 0)
        n += range;
    return minValue + n;
}

static void ResetSubMenuToDefaults(LayoutData& data, int subMenu)
{
    const LayoutData defaults = LayoutData_Default();
    switch (subMenu)
    {
        case UI_SUBMENU_DATETIME1: data.dateTime1 = defaults.dateTime1; break;
        case UI_SUBMENU_DATETIME2: data.dateTime2 = defaults.dateTime2; break;
        case UI_SUBMENU_PREFIX: data.prefix = defaults.prefix; break;
        case UI_SUBMENU_GAME_ID: data.gameId = defaults.gameId; break;
        case UI_SUBMENU_REGION: data.region = defaults.region; break;
        case UI_SUBMENU_VERSION: data.version = defaults.version; break;
        case UI_SUBMENU_CRC: data.crc = defaults.crc; break;
        case UI_SUBMENU_USERNAME: data.username = defaults.username; break;
        case UI_SUBMENU_BOXART: data.boxArt = defaults.boxArt; break;
        case UI_SUBMENU_ICON: data.icon = defaults.icon; break;
        case UI_SUBMENU_ROMNAME:
            data.romNameRow1 = defaults.romNameRow1;
            data.romNameRow2 = defaults.romNameRow2;
            data.romNameRow3 = defaults.romNameRow3;
            break;
        case UI_SUBMENU_FILENAME: data.fileName = defaults.fileName; break;
        default:
            break;
    }
}

int LayoutEditorBottomSheetView::ClampInt(int value, int minValue, int maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

void LayoutEditorBottomSheetView::EnsureThemeColorLoaded() const
{
    if (_themeColorLoaded)
        return;

    _themeColorLoaded = true;
    _themeColorR = 0;
    _themeColorG = 0;
    _themeColorB = 0;

    if (_appSettingsService == nullptr)
        return;

    const auto& appSettings = _appSettingsService->GetAppSettings();
    char path[128];
    mini_snprintf(path, sizeof(path), "/_pico/themes/%s/theme.json", appSettings.theme.GetString());

    const auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return;

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
        return;

    DynamicJsonDocument json(THEME_COLOR_JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileData.get(), fileSize) != DeserializationError::Ok)
        return;

    JsonObjectConst color = json[THEME_COLOR_KEY_PRIMARY_COLOR];
    if (color.isNull())
        return;

    _themeDarkMode = json[THEME_COLOR_KEY_DARK_THEME] | false;

    _themeColorR = ClampInt(color[THEME_COLOR_KEY_R] | 0, 0, 255);
    _themeColorG = ClampInt(color[THEME_COLOR_KEY_G] | 0, 0, 255);
    _themeColorB = ClampInt(color[THEME_COLOR_KEY_B] | 0, 0, 255);
}

void LayoutEditorBottomSheetView::ApplyThemeColorPreview()
{
    if (!_themeColorLoaded || _materialColorScheme == nullptr)
        return;

    MaterialColorSchemeFactory::FromPrimaryColor(
        Rgb<8, 8, 8>((u8)_themeColorR, (u8)_themeColorG, (u8)_themeColorB),
        _themeDarkMode,
        *const_cast<MaterialColorScheme*>(_materialColorScheme));
    _themePreviewApplied = true;
    RefreshThemeBackgroundPalettes();
}

void LayoutEditorBottomSheetView::RestoreThemeColorPreview()
{
    if (!_themePreviewApplied || _materialColorScheme == nullptr)
        return;

    *const_cast<MaterialColorScheme*>(_materialColorScheme) = _originalMaterialColorScheme;
    _themePreviewApplied = false;
    RefreshThemeBackgroundPalettes();
}

void LayoutEditorBottomSheetView::RefreshThemeBackgroundPalettes()
{
    mem_setVramHMapping(MEM_VRAM_H_LCDC);
    RgbMixer::MakeGradientPalette((u16*)0x06898020,
        _materialColorScheme->inverseOnSurface,
        _materialColorScheme->secondaryContainer);
    mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);

    auto scrimBlendColor = Rgb<8, 8, 8>(
        _materialColorScheme->inverseOnSurface.r + (_materialColorScheme->scrim.r - _materialColorScheme->inverseOnSurface.r) * 5 / 16,
        _materialColorScheme->inverseOnSurface.g + (_materialColorScheme->scrim.g - _materialColorScheme->inverseOnSurface.g) * 5 / 16,
        _materialColorScheme->inverseOnSurface.b + (_materialColorScheme->scrim.b - _materialColorScheme->inverseOnSurface.b) * 5 / 16);
    RgbMixer::MakeGradientPalette((u16*)GFX_PLTT_BG_MAIN, scrimBlendColor,
        _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    GFX_PLTT_BG_MAIN[0] = ColorConverter::ToGBGR565(_materialColorScheme->inverseOnSurface);
    GFX_PLTT_BG_MAIN[31] = ColorConverter::ToGBGR565(_materialColorScheme->scrim);
}

bool LayoutEditorBottomSheetView::SaveThemeColorToFile() const
{
    if (_appSettingsService == nullptr)
        return false;

    const auto& appSettings = _appSettingsService->GetAppSettings();
    char path[128];
    mini_snprintf(path, sizeof(path), "/_pico/themes/%s/theme.json", appSettings.theme.GetString());

    DynamicJsonDocument json(THEME_COLOR_JSON_RESERVED_SIZE);

    {
        const auto readFile = std::make_unique<File>();
        if (readFile->Open(path, FA_READ | FA_OPEN_EXISTING) == FR_OK)
        {
            u32 fileSize = readFile->GetSize();
            if (fileSize > 0)
            {
                std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
                u32 bytesRead = 0;
                if (readFile->Read(fileData.get(), fileSize, bytesRead) == FR_OK && bytesRead == fileSize)
                {
                    deserializeJson(json, fileData.get(), fileSize);
                }
            }
        }
    }

    JsonObject color = json[THEME_COLOR_KEY_PRIMARY_COLOR].to<JsonObject>();
    color[THEME_COLOR_KEY_R] = ClampInt(_themeColorR, 0, 255);
    color[THEME_COLOR_KEY_G] = ClampInt(_themeColorG, 0, 255);
    color[THEME_COLOR_KEY_B] = ClampInt(_themeColorB, 0, 255);
    json[THEME_COLOR_KEY_DARK_THEME] = _themeDarkMode;

    u32 outputSize = measureJsonPretty(json);
    if (outputSize == 0)
        return false;

    std::unique_ptr<u8[]> output(new(cache_align) u8[outputSize]);
    serializeJsonPretty(json, output.get(), outputSize);

    const auto writeFile = std::make_unique<File>();
    if (writeFile->Open(path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return false;

    u32 bytesWritten = 0;
    if (writeFile->Write(output.get(), outputSize, bytesWritten) != FR_OK || bytesWritten != outputSize)
        return false;

    return true;
}

bool LayoutEditorBottomSheetView::IsNonFocusableItem(int subMenu, int itemIdx) const
{
    return false;
}

int LayoutEditorBottomSheetView::GetNextFocusableItem(int subMenu, int itemIdx, int dir) const
{
    int count = GetSubMenuItemCount(subMenu);
    int idx = itemIdx + dir;
    while (idx >= 0 && idx < count)
    {
        if (!IsNonFocusableItem(subMenu, idx))
            return idx;
        idx += dir;
    }
    return -1;
}

int LayoutEditorBottomSheetView::GetChildIndentX(int subMenu, int itemIdx) const
{
    (void)subMenu;
    (void)itemIdx;
    return 0;
}

void LayoutEditorBottomSheetView::EnsureItemFocusVisible()
{
    auto focusToVisualRow = [&](int focusRow) -> int
    {
        if (focusRow <= kFocusResetMenu)
            return 0;
        if (focusRow == kFocusSubMenu)
            return 1;
        return 2 + (focusRow - kFocusItem0);
    };

    auto visualRowToFocus = [&](int visualRow) -> int
    {
        if (visualRow <= 0)
            return _topActionFocus;
        if (visualRow == 1)
            return kFocusSubMenu;
        return kFocusItem0 + (visualRow - 2);
    };

    const int totalVisualRows = 2 + GetSubMenuItemCount(_currentSubMenu);
    if (totalVisualRows <= 0)
    {
        _focusRow = 0;
        _topActionFocus = kFocusSlot;
        _itemScrollOffset = 0;
        return;
    }

    if (_focusRow <= kFocusResetMenu)
        _topActionFocus = ClampInt(_focusRow, kFocusSlot, kFocusResetMenu);

    int focusVisual = focusToVisualRow(_focusRow);
    focusVisual = ClampInt(focusVisual, 0, totalVisualRows - 1);

    if (focusVisual < _itemScrollOffset)
        _itemScrollOffset = focusVisual;
    else if (focusVisual >= _itemScrollOffset + kNumVisibleItems)
        _itemScrollOffset = focusVisual - (kNumVisibleItems - 1);

    ClampItemScroll();
    _focusRow = visualRowToFocus(focusVisual);
}

LayoutEditorBottomSheetView::LayoutEditorBottomSheetView(
    IRomBrowserController* controller,
    LayoutService* layoutService,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository,
    IAppSettingsService* appSettingsService)
    : _controller(controller)
    , _layoutService(layoutService)
    , _appSettingsService(appSettingsService)
    , _materialColorScheme(materialColorScheme)
    , _titleLabel(196, 16, 20, fontRepository->GetFont(FontType::Medium11))
    , _slotLabel(56, 16, 10, fontRepository->GetFont(FontType::Regular10))
    , _saveLabel(48, 16, 9, fontRepository->GetFont(FontType::Regular10))
    , _resetLabel(56, 16, 10, fontRepository->GetFont(FontType::Regular10))
    , _resetMenuLabel(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _subMenuLabel(196, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _itemName0(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName1(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName2(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName3(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName4(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName5(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName6(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName7(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemName8(96, 16, 16, fontRepository->GetFont(FontType::Regular10))
    , _itemValue0(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue1(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue2(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue3(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue4(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue5(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue6(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue7(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _itemValue8(128, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _scrollHintLabel(128, 10, 8, fontRepository->GetFont(FontType::Regular10))
    , _originalMaterialColorScheme(*materialColorScheme)
{
    AddChildTail(&_titleLabel);
    AddChildTail(&_slotLabel);
    AddChildTail(&_saveLabel);
    AddChildTail(&_resetLabel);
    AddChildTail(&_resetMenuLabel);
    AddChildTail(&_subMenuLabel);
    AddChildTail(&_itemName0);
    AddChildTail(&_itemName1);
    AddChildTail(&_itemName2);
    AddChildTail(&_itemName3);
    AddChildTail(&_itemName4);
    AddChildTail(&_itemName5);
    AddChildTail(&_itemName6);
    AddChildTail(&_itemName7);
    AddChildTail(&_itemName8);
    AddChildTail(&_itemValue0);
    AddChildTail(&_itemValue1);
    AddChildTail(&_itemValue2);
    AddChildTail(&_itemValue3);
    AddChildTail(&_itemValue4);
    AddChildTail(&_itemValue5);
    AddChildTail(&_itemValue6);
    AddChildTail(&_itemValue7);
    AddChildTail(&_itemValue8);
    AddChildTail(&_scrollHintLabel);

    _titleLabel.SetText("Layout Editor");
}

void LayoutEditorBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);
}

void LayoutEditorBottomSheetView::Focus(FocusManager& focusManager)
{
    focusManager.Unfocus();
}

void LayoutEditorBottomSheetView::OnDismissed()
{
    if (_themePreviewApplied)
    {
        RestoreThemeColorPreview();
    }
    if (_themeColorDirty)
    {
        _themeColorDirty = false;
        _themeColorLoaded = false;
    }
    _controller->HideLayoutEditor();
}

Label2DView* LayoutEditorBottomSheetView::GetItemNameLabel(int visRow)
{
    switch (visRow)
    {
        case 0: return &_itemName0;
        case 1: return &_itemName1;
        case 2: return &_itemName2;
        case 3: return &_itemName3;
        case 4: return &_itemName4;
        case 5: return &_itemName5;
        case 6: return &_itemName6;
        case 7: return &_itemName7;
        case 8: return &_itemName8;
        default: return &_itemName0;
    }
}

Label2DView* LayoutEditorBottomSheetView::GetItemValueLabel(int visRow)
{
    switch (visRow)
    {
        case 0: return &_itemValue0;
        case 1: return &_itemValue1;
        case 2: return &_itemValue2;
        case 3: return &_itemValue3;
        case 4: return &_itemValue4;
        case 5: return &_itemValue5;
        case 6: return &_itemValue6;
        case 7: return &_itemValue7;
        case 8: return &_itemValue8;
        default: return &_itemValue0;
    }
}

int LayoutEditorBottomSheetView::GetSubMenuCount() const
{
    return UI_SUBMENU_COUNT;
}

const char* LayoutEditorBottomSheetView::GetSubMenuName(int uiSubMenu) const
{
    if (uiSubMenu < 0 || uiSubMenu >= GetSubMenuCount())
        return "?";
    return kUiSubMenuNames[uiSubMenu];
}

int LayoutEditorBottomSheetView::GetSubMenuItemCount(int subMenu) const
{
    if ((u32)subMenu < UI_SUBMENU_COUNT)
        return kSubMenuItemCounts[subMenu];
    return 0;
}

const char* LayoutEditorBottomSheetView::GetItemName(int subMenu, int itemIdx) const
{
    switch (subMenu)
    {
        case UI_SUBMENU_DATETIME1:
        case UI_SUBMENU_DATETIME2:
            return (u32)itemIdx < 9 ? kItemNamesDateTime[itemIdx] : "";
        case UI_SUBMENU_PREFIX:
            return (u32)itemIdx < 11 ? kItemNamesPrefix[itemIdx] : "";
        case UI_SUBMENU_GAME_ID:
            return (u32)itemIdx < 15 ? kItemNamesGameId[itemIdx] : "";
        case UI_SUBMENU_REGION:
            return (u32)itemIdx < 8 ? kItemNamesRegion[itemIdx] : "";
        case UI_SUBMENU_VERSION:
            return (u32)itemIdx < 7 ? kItemNamesElement[itemIdx] : "";
        case UI_SUBMENU_CRC:
            return (u32)itemIdx < 7 ? kItemNamesElement[itemIdx] : "";
        case UI_SUBMENU_USERNAME:
            return (u32)itemIdx < 7 ? kItemNamesElement[itemIdx] : "";
        case UI_SUBMENU_BOXART:
        case UI_SUBMENU_ICON:
            return (u32)itemIdx < 3 ? kItemNamesNoFont[itemIdx] : "";
        case UI_SUBMENU_ROMNAME:
            return (u32)itemIdx < 8 ? kItemNamesRomName[itemIdx] : "";
        case UI_SUBMENU_FILENAME:
            return (u32)itemIdx < 9 ? kItemNamesFileName[itemIdx] : "";
        case UI_SUBMENU_THEMECOLOR:
            return (u32)itemIdx < 5 ? kItemNamesThemeColor[itemIdx] : "";
        default:
            return "";
    }
}

void LayoutEditorBottomSheetView::GetItemValueText(
    int subMenu, int itemIdx, char* buf, u32 bufLen) const
{
    if (bufLen == 0) return;
    buf[0] = '\0';

    const LayoutData& d = _layoutService->GetCurrentLayout();

    #define WRITE_VISIBLE(v) mini_snprintf(buf, bufLen, "%s", (v) ? "On" : "Off")
    #define WRITE_COORD(v)   mini_snprintf(buf, bufLen, "%d", (int)(v))
    #define WRITE_FONT(v)    mini_snprintf(buf, bufLen, "%s", kLayoutFontNames[(v) % LAYOUT_FONT_COUNT])

    switch (subMenu)
    {
        case UI_SUBMENU_DATETIME1:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.dateTime1.visible); break;
                case 1: WRITE_COORD(d.dateTime1.y); break;
                case 2: WRITE_COORD(d.dateTime1.x); break;
                case 3: mini_snprintf(buf, bufLen, "%s", kLayoutFormatNames[d.dateTime1.format % LAYOUT_FORMAT_COUNT]); break;
                case 4: mini_snprintf(buf, bufLen, "%s", kLayoutSeparatorNames[d.dateTime1.separator % LAYOUT_SEP_COUNT]); break;
                case 5: WRITE_FONT(d.dateTime1.font); break;
                case 6: WRITE_COORD(d.dateTime1.colorR); break;
                case 7: WRITE_COORD(d.dateTime1.colorG); break;
                case 8: WRITE_COORD(d.dateTime1.colorB); break;
            }
            break;
        case UI_SUBMENU_DATETIME2:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.dateTime2.visible); break;
                case 1: WRITE_COORD(d.dateTime2.y); break;
                case 2: WRITE_COORD(d.dateTime2.x); break;
                case 3: mini_snprintf(buf, bufLen, "%s", kLayoutFormatNames[d.dateTime2.format % LAYOUT_FORMAT_COUNT]); break;
                case 4: mini_snprintf(buf, bufLen, "%s", kLayoutSeparatorNames[d.dateTime2.separator % LAYOUT_SEP_COUNT]); break;
                case 5: WRITE_FONT(d.dateTime2.font); break;
                case 6: WRITE_COORD(d.dateTime2.colorR); break;
                case 7: WRITE_COORD(d.dateTime2.colorG); break;
                case 8: WRITE_COORD(d.dateTime2.colorB); break;
            }
            break;
        case UI_SUBMENU_PREFIX:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.prefix.visible); break;
                case 1: WRITE_COORD(d.prefix.x); break;
                case 2: WRITE_COORD(d.prefix.y); break;
                case 3: WRITE_FONT(d.prefix.font); break;
                case 4: WRITE_VISIBLE(d.prefix.trailingDash); break;
                case 5: mini_snprintf(buf, bufLen, "%s", kLayoutPrefixGbaModeNames[d.prefix.gbaPrefixMode % LAYOUT_PREFIX_GBA_COUNT]); break;
                case 6: mini_snprintf(buf, bufLen, "%s", kLayoutPrefixNtrModeNames[d.prefix.ntrPrefixMode % LAYOUT_PREFIX_NTR_COUNT]); break;
                case 7: mini_snprintf(buf, bufLen, "%s", kLayoutPrefixTwlModeNames[d.prefix.twlPrefixMode % LAYOUT_PREFIX_TWL_COUNT]); break;
                case 8: WRITE_COORD(d.prefix.colorR); break;
                case 9: WRITE_COORD(d.prefix.colorG); break;
                case 10: WRITE_COORD(d.prefix.colorB); break;
            }
            break;
        case UI_SUBMENU_GAME_ID:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.gameId.visible); break;
                case 1: WRITE_COORD(d.gameId.x); break;
                case 2: WRITE_COORD(d.gameId.y); break;
                case 3: WRITE_FONT(d.gameId.font); break;
                case 4: WRITE_VISIBLE(d.gameId.trailingDash); break;
                case 5: WRITE_VISIBLE(d.gameId.showLabelText); break;
                case 6: WRITE_FONT(d.gameId.labelFont); break;
                case 7: WRITE_COORD(d.gameId.labelX); break;
                case 8: WRITE_COORD(d.gameId.labelY); break;
                case 9: WRITE_COORD(d.gameId.labelColorR); break;
                case 10: WRITE_COORD(d.gameId.labelColorG); break;
                case 11: WRITE_COORD(d.gameId.labelColorB); break;
                case 12: WRITE_COORD(d.gameId.colorR); break;
                case 13: WRITE_COORD(d.gameId.colorG); break;
                case 14: WRITE_COORD(d.gameId.colorB); break;
            }
            break;
        case UI_SUBMENU_REGION:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.region.visible); break;
                case 1: WRITE_COORD(d.region.x); break;
                case 2: WRITE_COORD(d.region.y); break;
                case 3: WRITE_FONT(d.region.font); break;
                case 4: WRITE_VISIBLE(d.region.trailingDash); break;
                case 5: WRITE_COORD(d.region.colorR); break;
                case 6: WRITE_COORD(d.region.colorG); break;
                case 7: WRITE_COORD(d.region.colorB); break;
            }
            break;
        case UI_SUBMENU_VERSION:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.version.visible); break;
                case 1: WRITE_COORD(d.version.y); break;
                case 2: WRITE_COORD(d.version.x); break;
                case 3: WRITE_FONT(d.version.font); break;
                case 4: WRITE_COORD(d.version.colorR); break;
                case 5: WRITE_COORD(d.version.colorG); break;
                case 6: WRITE_COORD(d.version.colorB); break;
            }
            break;
        case UI_SUBMENU_CRC:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.crc.visible); break;
                case 1: WRITE_COORD(d.crc.y); break;
                case 2: WRITE_COORD(d.crc.x); break;
                case 3: WRITE_FONT(d.crc.font); break;
                case 4: WRITE_COORD(d.crc.colorR); break;
                case 5: WRITE_COORD(d.crc.colorG); break;
                case 6: WRITE_COORD(d.crc.colorB); break;
            }
            break;
        case UI_SUBMENU_USERNAME:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.username.visible); break;
                case 1: WRITE_COORD(d.username.y); break;
                case 2: WRITE_COORD(d.username.x); break;
                case 3: WRITE_FONT(d.username.font); break;
                case 4: WRITE_COORD(d.username.colorR); break;
                case 5: WRITE_COORD(d.username.colorG); break;
                case 6: WRITE_COORD(d.username.colorB); break;
            }
            break;
        case UI_SUBMENU_BOXART:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.boxArt.visible); break;
                case 1: WRITE_COORD(d.boxArt.y); break;
                case 2: WRITE_COORD(d.boxArt.x); break;
            }
            break;
        case UI_SUBMENU_ICON:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.icon.visible); break;
                case 1: WRITE_COORD(d.icon.y); break;
                case 2: WRITE_COORD(d.icon.x); break;
            }
            break;
        case UI_SUBMENU_ROMNAME:
            switch (itemIdx)
            {
                case 0: mini_snprintf(buf, bufLen, "%d", _romNameSelectedLine + 1); break;
                case 1:
                    if (_romNameSelectedLine == 0) WRITE_VISIBLE(d.romNameRow1.visible);
                    else if (_romNameSelectedLine == 1) WRITE_VISIBLE(d.romNameRow2.visible);
                    else WRITE_VISIBLE(d.romNameRow3.visible);
                    break;
                case 2:
                    if (_romNameSelectedLine == 0) WRITE_COORD(d.romNameRow1.x);
                    else if (_romNameSelectedLine == 1) WRITE_COORD(d.romNameRow2.x);
                    else WRITE_COORD(d.romNameRow3.x);
                    break;
                case 3:
                    if (_romNameSelectedLine == 0) WRITE_COORD(d.romNameRow1.y);
                    else if (_romNameSelectedLine == 1) WRITE_COORD(d.romNameRow2.y);
                    else WRITE_COORD(d.romNameRow3.y);
                    break;
                case 4:
                    if (_romNameSelectedLine == 0) WRITE_FONT(d.romNameRow1.font);
                    else if (_romNameSelectedLine == 1) WRITE_FONT(d.romNameRow2.font);
                    else WRITE_FONT(d.romNameRow3.font);
                    break;
                case 5:
                    if (_romNameSelectedLine == 0) WRITE_COORD(d.romNameRow1.colorR);
                    else if (_romNameSelectedLine == 1) WRITE_COORD(d.romNameRow2.colorR);
                    else WRITE_COORD(d.romNameRow3.colorR);
                    break;
                case 6:
                    if (_romNameSelectedLine == 0) WRITE_COORD(d.romNameRow1.colorG);
                    else if (_romNameSelectedLine == 1) WRITE_COORD(d.romNameRow2.colorG);
                    else WRITE_COORD(d.romNameRow3.colorG);
                    break;
                case 7:
                    if (_romNameSelectedLine == 0) WRITE_COORD(d.romNameRow1.colorB);
                    else if (_romNameSelectedLine == 1) WRITE_COORD(d.romNameRow2.colorB);
                    else WRITE_COORD(d.romNameRow3.colorB);
                    break;
            }
            break;
        case UI_SUBMENU_FILENAME:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.fileName.visible); break;
                case 1: WRITE_COORD(d.fileName.y); break;
                case 2: WRITE_COORD(d.fileName.x); break;
                case 3: WRITE_FONT(d.fileName.font); break;
                case 4: mini_snprintf(buf, bufLen, "%s", d.fileName.scroll ? "On" : "Off"); break;
                case 5: WRITE_COORD(d.fileName.scrollSpeed); break;
                case 6: WRITE_COORD(d.fileName.colorR); break;
                case 7: WRITE_COORD(d.fileName.colorG); break;
                case 8: WRITE_COORD(d.fileName.colorB); break;
            }
            break;
        case UI_SUBMENU_THEMECOLOR:
            EnsureThemeColorLoaded();
            switch (itemIdx)
            {
                case 0: mini_snprintf(buf, bufLen, "%d", _themeColorR); break;
                case 1: mini_snprintf(buf, bufLen, "%d", _themeColorG); break;
                case 2: mini_snprintf(buf, bufLen, "%d", _themeColorB); break;
                case 3: mini_snprintf(buf, bufLen, "%s", _themeDarkMode ? "True" : "False"); break;
            }
            break;
    }

    #undef WRITE_VISIBLE
    #undef WRITE_COORD
    #undef WRITE_FONT
}

void LayoutEditorBottomSheetView::ChangeItemValue(int subMenu, int itemIdx, int delta)
{
    LayoutData& d = _layoutService->GetCurrentLayoutMutable();

    auto cycleU8 = [](u8& v, int dv, u8 count) {
        int n = ((int)(v % count) + dv % (int)count + (int)count) % (int)count;
        v = (u8)n;
    };
    auto toggleU8 = [](u8& v) { v = v ? 0 : 1; };

    auto changeX = [&](s16& v) { ChangeWrappedCoord(v, delta, -30, 260); };
    auto changeY = [&](s16& v) { ChangeWrappedCoord(v, delta, -20, 200); };
    auto changeColor = [&](u8& v) { v = (u8)WrapRange((int)v + delta, 0, 255); };

    switch (subMenu)
    {
        case UI_SUBMENU_DATETIME1:
            switch (itemIdx)
            {
                case 0: toggleU8(d.dateTime1.visible); break;
                case 1: changeY(d.dateTime1.y); break;
                case 2: changeX(d.dateTime1.x); break;
                case 3: cycleU8(d.dateTime1.format, delta, LAYOUT_FORMAT_COUNT); break;
                case 4: cycleU8(d.dateTime1.separator, delta, LAYOUT_SEP_COUNT); break;
                case 5: cycleU8(d.dateTime1.font, delta, LAYOUT_FONT_COUNT); break;
                case 6: changeColor(d.dateTime1.colorR); break;
                case 7: changeColor(d.dateTime1.colorG); break;
                case 8: changeColor(d.dateTime1.colorB); break;
            }
            break;
        case UI_SUBMENU_DATETIME2:
            switch (itemIdx)
            {
                case 0: toggleU8(d.dateTime2.visible); break;
                case 1: changeY(d.dateTime2.y); break;
                case 2: changeX(d.dateTime2.x); break;
                case 3: cycleU8(d.dateTime2.format, delta, LAYOUT_FORMAT_COUNT); break;
                case 4: cycleU8(d.dateTime2.separator, delta, LAYOUT_SEP_COUNT); break;
                case 5: cycleU8(d.dateTime2.font, delta, LAYOUT_FONT_COUNT); break;
                case 6: changeColor(d.dateTime2.colorR); break;
                case 7: changeColor(d.dateTime2.colorG); break;
                case 8: changeColor(d.dateTime2.colorB); break;
            }
            break;
        case UI_SUBMENU_PREFIX:
            switch (itemIdx)
            {
                case 0: toggleU8(d.prefix.visible); break;
                case 1: changeX(d.prefix.x); break;
                case 2: changeY(d.prefix.y); break;
                case 3: cycleU8(d.prefix.font, delta, LAYOUT_FONT_COUNT); break;
                case 4: toggleU8(d.prefix.trailingDash); break;
                case 5: cycleU8(d.prefix.gbaPrefixMode, delta, LAYOUT_PREFIX_GBA_COUNT); break;
                case 6: cycleU8(d.prefix.ntrPrefixMode, delta, LAYOUT_PREFIX_NTR_COUNT); break;
                case 7: cycleU8(d.prefix.twlPrefixMode, delta, LAYOUT_PREFIX_TWL_COUNT); break;
                case 8: changeColor(d.prefix.colorR); break;
                case 9: changeColor(d.prefix.colorG); break;
                case 10: changeColor(d.prefix.colorB); break;
            }
            break;
        case UI_SUBMENU_GAME_ID:
            switch (itemIdx)
            {
                case 0: toggleU8(d.gameId.visible); break;
                case 1: changeX(d.gameId.x); break;
                case 2: changeY(d.gameId.y); break;
                case 3: cycleU8(d.gameId.font, delta, LAYOUT_FONT_COUNT); break;
                case 4: toggleU8(d.gameId.trailingDash); break;
                case 5: toggleU8(d.gameId.showLabelText); break;
                case 6: cycleU8(d.gameId.labelFont, delta, LAYOUT_FONT_COUNT); break;
                case 7: changeX(d.gameId.labelX); break;
                case 8: changeY(d.gameId.labelY); break;
                case 9: changeColor(d.gameId.labelColorR); break;
                case 10: changeColor(d.gameId.labelColorG); break;
                case 11: changeColor(d.gameId.labelColorB); break;
                case 12: changeColor(d.gameId.colorR); break;
                case 13: changeColor(d.gameId.colorG); break;
                case 14: changeColor(d.gameId.colorB); break;
            }
            break;
        case UI_SUBMENU_REGION:
            switch (itemIdx)
            {
                case 0: toggleU8(d.region.visible); break;
                case 1: changeX(d.region.x); break;
                case 2: changeY(d.region.y); break;
                case 3: cycleU8(d.region.font, delta, LAYOUT_FONT_COUNT); break;
                case 4: toggleU8(d.region.trailingDash); break;
                case 5: changeColor(d.region.colorR); break;
                case 6: changeColor(d.region.colorG); break;
                case 7: changeColor(d.region.colorB); break;
            }
            break;
        case UI_SUBMENU_VERSION:
            switch (itemIdx)
            {
                case 0: toggleU8(d.version.visible); break;
                case 1: changeY(d.version.y); break;
                case 2: changeX(d.version.x); break;
                case 3: cycleU8(d.version.font, delta, LAYOUT_FONT_COUNT); break;
                case 4: changeColor(d.version.colorR); break;
                case 5: changeColor(d.version.colorG); break;
                case 6: changeColor(d.version.colorB); break;
            }
            break;
        case UI_SUBMENU_CRC:
            switch (itemIdx)
            {
                case 0: toggleU8(d.crc.visible); break;
                case 1: changeY(d.crc.y); break;
                case 2: changeX(d.crc.x); break;
                case 3: cycleU8(d.crc.font, delta, LAYOUT_FONT_COUNT); break;
                case 4: changeColor(d.crc.colorR); break;
                case 5: changeColor(d.crc.colorG); break;
                case 6: changeColor(d.crc.colorB); break;
            }
            break;
        case UI_SUBMENU_USERNAME:
            switch (itemIdx)
            {
                case 0: toggleU8(d.username.visible); break;
                case 1: changeY(d.username.y); break;
                case 2: changeX(d.username.x); break;
                case 3: cycleU8(d.username.font, delta, LAYOUT_FONT_COUNT); break;
                case 4: changeColor(d.username.colorR); break;
                case 5: changeColor(d.username.colorG); break;
                case 6: changeColor(d.username.colorB); break;
            }
            break;
        case UI_SUBMENU_BOXART:
            switch (itemIdx)
            {
                case 0: toggleU8(d.boxArt.visible); break;
                case 1: changeY(d.boxArt.y); break;
                case 2: changeX(d.boxArt.x); break;
            }
            break;
        case UI_SUBMENU_ICON:
            switch (itemIdx)
            {
                case 0: toggleU8(d.icon.visible); break;
                case 1: changeY(d.icon.y); break;
                case 2: changeX(d.icon.x); break;
            }
            break;
        case UI_SUBMENU_ROMNAME:
            switch (itemIdx)
            {
                case 0:
                    _romNameSelectedLine = WrapRange(_romNameSelectedLine + delta, 0, 2);
                    break;
                case 1:
                    if (_romNameSelectedLine == 0) toggleU8(d.romNameRow1.visible);
                    else if (_romNameSelectedLine == 1) toggleU8(d.romNameRow2.visible);
                    else toggleU8(d.romNameRow3.visible);
                    break;
                case 2:
                    if (_romNameSelectedLine == 0) changeX(d.romNameRow1.x);
                    else if (_romNameSelectedLine == 1) changeX(d.romNameRow2.x);
                    else changeX(d.romNameRow3.x);
                    break;
                case 3:
                    if (_romNameSelectedLine == 0) changeY(d.romNameRow1.y);
                    else if (_romNameSelectedLine == 1) changeY(d.romNameRow2.y);
                    else changeY(d.romNameRow3.y);
                    break;
                case 4:
                    if (_romNameSelectedLine == 0) cycleU8(d.romNameRow1.font, delta, LAYOUT_FONT_COUNT);
                    else if (_romNameSelectedLine == 1) cycleU8(d.romNameRow2.font, delta, LAYOUT_FONT_COUNT);
                    else cycleU8(d.romNameRow3.font, delta, LAYOUT_FONT_COUNT);
                    break;
                case 5:
                    if (_romNameSelectedLine == 0) changeColor(d.romNameRow1.colorR);
                    else if (_romNameSelectedLine == 1) changeColor(d.romNameRow2.colorR);
                    else changeColor(d.romNameRow3.colorR);
                    break;
                case 6:
                    if (_romNameSelectedLine == 0) changeColor(d.romNameRow1.colorG);
                    else if (_romNameSelectedLine == 1) changeColor(d.romNameRow2.colorG);
                    else changeColor(d.romNameRow3.colorG);
                    break;
                case 7:
                    if (_romNameSelectedLine == 0) changeColor(d.romNameRow1.colorB);
                    else if (_romNameSelectedLine == 1) changeColor(d.romNameRow2.colorB);
                    else changeColor(d.romNameRow3.colorB);
                    break;
            }
            break;
        case UI_SUBMENU_FILENAME:
            switch (itemIdx)
            {
                case 0: toggleU8(d.fileName.visible); break;
                case 1: changeY(d.fileName.y); break;
                case 2: changeX(d.fileName.x); break;
                case 3: cycleU8(d.fileName.font, delta, LAYOUT_FONT_COUNT); break;
                case 4: toggleU8(d.fileName.scroll); break;
                case 5: d.fileName.scrollSpeed = (u8)WrapRange((int)d.fileName.scrollSpeed + delta, 1, 20); break;
                case 6: changeColor(d.fileName.colorR); break;
                case 7: changeColor(d.fileName.colorG); break;
                case 8: changeColor(d.fileName.colorB); break;
            }
            break;
        case UI_SUBMENU_THEMECOLOR:
            EnsureThemeColorLoaded();
            if (itemIdx == 0) _themeColorR = WrapRange(_themeColorR + delta, 0, 255);
            if (itemIdx == 1) _themeColorG = WrapRange(_themeColorG + delta, 0, 255);
            if (itemIdx == 2) _themeColorB = WrapRange(_themeColorB + delta, 0, 255);
            if (itemIdx == 3) _themeDarkMode = !_themeDarkMode;
            _themeColorDirty = true;
            break;
    }
}

bool LayoutEditorBottomSheetView::GetChoiceListForCurrentFocus(
    ChoiceKind& kind, int& count, int& selectedValue) const
{
    kind = ChoiceKind::None;
    count = 0;
    selectedValue = 0;

    if (_focusRow == kFocusSlot)
    {
        kind = ChoiceKind::Slot;
        count = LAYOUT_MAX_SLOTS;
        selectedValue = (int)_layoutService->GetCurrentSlot() - 1;
        if (selectedValue < 0) selectedValue = 0;
        return true;
    }

    if (_focusRow == kFocusSubMenu)
    {
        kind = ChoiceKind::SubMenu;
        count = GetSubMenuCount();
        selectedValue = _currentSubMenu;
        return true;
    }

    if (_focusRow < kFocusItem0)
        return false;

    int itemIdx = _focusRow - kFocusItem0;
    if (itemIdx >= GetSubMenuItemCount(_currentSubMenu))
        return false;

    const LayoutData& d = _layoutService->GetCurrentLayout();

    switch (_currentSubMenu)
    {
        case UI_SUBMENU_DATETIME1:
            if (itemIdx == 3) { kind = ChoiceKind::Format; count = LAYOUT_FORMAT_COUNT; selectedValue = d.dateTime1.format; return true; }
            if (itemIdx == 4) { kind = ChoiceKind::Separator; count = LAYOUT_SEP_COUNT; selectedValue = d.dateTime1.separator; return true; }
            if (itemIdx == 5) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.dateTime1.font; return true; }
            break;
        case UI_SUBMENU_DATETIME2:
            if (itemIdx == 3) { kind = ChoiceKind::Format; count = LAYOUT_FORMAT_COUNT; selectedValue = d.dateTime2.format; return true; }
            if (itemIdx == 4) { kind = ChoiceKind::Separator; count = LAYOUT_SEP_COUNT; selectedValue = d.dateTime2.separator; return true; }
            if (itemIdx == 5) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.dateTime2.font; return true; }
            break;
        case UI_SUBMENU_PREFIX:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.prefix.font; return true; }
            if (itemIdx == 5) { kind = ChoiceKind::PrefixGbaMode; count = LAYOUT_PREFIX_GBA_COUNT; selectedValue = d.prefix.gbaPrefixMode; return true; }
            if (itemIdx == 6) { kind = ChoiceKind::PrefixNtrMode; count = LAYOUT_PREFIX_NTR_COUNT; selectedValue = d.prefix.ntrPrefixMode; return true; }
            if (itemIdx == 7) { kind = ChoiceKind::PrefixTwlMode; count = LAYOUT_PREFIX_TWL_COUNT; selectedValue = d.prefix.twlPrefixMode; return true; }
            break;
        case UI_SUBMENU_GAME_ID:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.gameId.font; return true; }
            if (itemIdx == 6) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.gameId.labelFont; return true; }
            break;
        case UI_SUBMENU_REGION:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.region.font; return true; }
            break;
        case UI_SUBMENU_VERSION:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.version.font; return true; }
            break;
        case UI_SUBMENU_CRC:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.crc.font; return true; }
            break;
        case UI_SUBMENU_USERNAME:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.username.font; return true; }
            break;
        case UI_SUBMENU_ROMNAME:
            if (itemIdx == 4)
            {
                kind = ChoiceKind::Font;
                count = LAYOUT_FONT_COUNT;
                selectedValue = (_romNameSelectedLine == 0) ? d.romNameRow1.font
                    : (_romNameSelectedLine == 1 ? d.romNameRow2.font : d.romNameRow3.font);
                return true;
            }
            break;
        case UI_SUBMENU_FILENAME:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.fileName.font; return true; }
            break;
    }

    return false;
}

const char* LayoutEditorBottomSheetView::GetChoiceLabel(ChoiceKind kind, int choiceIdx) const
{
    switch (kind)
    {
        case ChoiceKind::Slot:
        {
            static char slotLabel[12];
            mini_snprintf(slotLabel, sizeof(slotLabel), "Slot %d", choiceIdx + 1);
            return slotLabel;
        }
        case ChoiceKind::SubMenu:
            return GetSubMenuName(choiceIdx);
        case ChoiceKind::Format:
            return (u32)choiceIdx < LAYOUT_FORMAT_COUNT ? kLayoutFormatNames[choiceIdx] : "";
        case ChoiceKind::Separator:
            return (u32)choiceIdx < LAYOUT_SEP_COUNT ? kLayoutSeparatorNames[choiceIdx] : "";
        case ChoiceKind::Font:
            return (u32)choiceIdx < LAYOUT_FONT_COUNT ? kLayoutFontNames[choiceIdx] : "";
        case ChoiceKind::PrefixGbaMode:
            return (u32)choiceIdx < LAYOUT_PREFIX_GBA_COUNT ? kLayoutPrefixGbaModeNames[choiceIdx] : "";
        case ChoiceKind::PrefixNtrMode:
            return (u32)choiceIdx < LAYOUT_PREFIX_NTR_COUNT ? kLayoutPrefixNtrModeNames[choiceIdx] : "";
        case ChoiceKind::PrefixTwlMode:
            return (u32)choiceIdx < LAYOUT_PREFIX_TWL_COUNT ? kLayoutPrefixTwlModeNames[choiceIdx] : "";
        default:
            return "";
    }
}

void LayoutEditorBottomSheetView::ApplyChoiceValue(ChoiceKind kind, int valueIdx)
{
    LayoutData& d = _layoutService->GetCurrentLayoutMutable();

    if (kind == ChoiceKind::Slot)
    {
        _layoutService->SetCurrentSlot((u32)(valueIdx + 1));
        EnsureItemFocusVisible();
        ClampItemScroll();
        return;
    }

    if (kind == ChoiceKind::SubMenu)
    {
        _currentSubMenu = valueIdx;
        EnsureItemFocusVisible();
        ClampItemScroll();
        return;
    }

    if (_choiceTargetIdx < 0)
        return;

    switch (_currentSubMenu)
    {
        case UI_SUBMENU_DATETIME1:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Format) d.dateTime1.format = (u8)valueIdx;
            if (_choiceTargetIdx == 4 && kind == ChoiceKind::Separator) d.dateTime1.separator = (u8)valueIdx;
            if (_choiceTargetIdx == 5 && kind == ChoiceKind::Font) d.dateTime1.font = (u8)valueIdx;
            break;
        case UI_SUBMENU_DATETIME2:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Format) d.dateTime2.format = (u8)valueIdx;
            if (_choiceTargetIdx == 4 && kind == ChoiceKind::Separator) d.dateTime2.separator = (u8)valueIdx;
            if (_choiceTargetIdx == 5 && kind == ChoiceKind::Font) d.dateTime2.font = (u8)valueIdx;
            break;
        case UI_SUBMENU_PREFIX:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.prefix.font = (u8)valueIdx;
            if (_choiceTargetIdx == 5 && kind == ChoiceKind::PrefixGbaMode) d.prefix.gbaPrefixMode = (u8)valueIdx;
            if (_choiceTargetIdx == 6 && kind == ChoiceKind::PrefixNtrMode) d.prefix.ntrPrefixMode = (u8)valueIdx;
            if (_choiceTargetIdx == 7 && kind == ChoiceKind::PrefixTwlMode) d.prefix.twlPrefixMode = (u8)valueIdx;
            break;
        case UI_SUBMENU_GAME_ID:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.gameId.font = (u8)valueIdx;
            if (_choiceTargetIdx == 6 && kind == ChoiceKind::Font) d.gameId.labelFont = (u8)valueIdx;
            break;
        case UI_SUBMENU_REGION:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.region.font = (u8)valueIdx;
            break;
        case UI_SUBMENU_VERSION:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.version.font = (u8)valueIdx;
            break;
        case UI_SUBMENU_CRC:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.crc.font = (u8)valueIdx;
            break;
        case UI_SUBMENU_USERNAME:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.username.font = (u8)valueIdx;
            break;
        case UI_SUBMENU_ROMNAME:
            if (_choiceTargetIdx == 4 && kind == ChoiceKind::Font)
            {
                if (_romNameSelectedLine == 0) d.romNameRow1.font = (u8)valueIdx;
                else if (_romNameSelectedLine == 1) d.romNameRow2.font = (u8)valueIdx;
                else d.romNameRow3.font = (u8)valueIdx;
            }
            break;
        case UI_SUBMENU_FILENAME:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.fileName.font = (u8)valueIdx;
            break;
    }
}

void LayoutEditorBottomSheetView::BeginChoice(
    ChoiceKind kind, int count, int selectedValue, int targetIdx)
{
    if (count <= 0)
        return;

    _isChoosing = true;
    _focusBeforeChoice = _focusRow;
    _choiceKind = kind;
    _choiceCount = count;
    _choiceSelected = selectedValue;
    if (_choiceSelected < 0) _choiceSelected = 0;
    if (_choiceSelected >= _choiceCount) _choiceSelected = _choiceCount - 1;

    _choiceScroll = 0;
    if (_choiceCount > kNumVisibleItems)
    {
        _choiceScroll = _choiceSelected - (kNumVisibleItems / 2);
        if (_choiceScroll < 0) _choiceScroll = 0;
        int maxScroll = _choiceCount - kNumVisibleItems;
        if (_choiceScroll > maxScroll) _choiceScroll = maxScroll;
    }

    _choiceTargetIdx = targetIdx;
}

void LayoutEditorBottomSheetView::CancelChoice()
{
    _isChoosing = false;
    _choiceKind = ChoiceKind::None;
    _choiceCount = 0;
    _choiceSelected = 0;
    _choiceScroll = 0;
    _choiceTargetIdx = -1;
    _focusRow = _focusBeforeChoice;
}

void LayoutEditorBottomSheetView::ConfirmChoice()
{
    if (!_isChoosing)
        return;

    ApplyChoiceValue(_choiceKind, _choiceSelected);

    _isChoosing = false;
    _choiceKind = ChoiceKind::None;
    _choiceCount = 0;
    _choiceSelected = 0;
    _choiceScroll = 0;
    _choiceTargetIdx = -1;
    _focusRow = _focusBeforeChoice;
}

void LayoutEditorBottomSheetView::ClampItemScroll()
{
    int totalVisualRows = 2 + GetSubMenuItemCount(_currentSubMenu);
    int maxScroll = totalVisualRows - kNumVisibleItems;
    if (maxScroll < 0) maxScroll = 0;
    if (_itemScrollOffset < 0) _itemScrollOffset = 0;
    if (_itemScrollOffset > maxScroll) _itemScrollOffset = maxScroll;
}

void LayoutEditorBottomSheetView::UpdateAllLabels()
{
    EnsureItemFocusVisible();

    _titleLabel.SetText("Layout Editor");
    _titleLabel.SetPosition(LE_TITLE_X, _position.y + LE_TITLE_Y);

    if (_isChoosing)
    {
        switch (_choiceKind)
        {
            case ChoiceKind::Slot: _titleLabel.SetText("Select Slot"); break;
            case ChoiceKind::SubMenu: _titleLabel.SetText("Select Menu"); break;
            case ChoiceKind::Format: _titleLabel.SetText("Select Format"); break;
            case ChoiceKind::Separator: _titleLabel.SetText("Select Separator"); break;
            case ChoiceKind::Font: _titleLabel.SetText("Select Font"); break;
            case ChoiceKind::PrefixGbaMode: _titleLabel.SetText("Select GBA Prefix"); break;
            case ChoiceKind::PrefixNtrMode: _titleLabel.SetText("Select NTR Prefix"); break;
            case ChoiceKind::PrefixTwlMode: _titleLabel.SetText("Select TWL Prefix"); break;
            default: _titleLabel.SetText("Select Value"); break;
        }

        _slotLabel.SetText("");
        _saveLabel.SetText("");
        _resetLabel.SetText("");
        _resetMenuLabel.SetText("");
        _subMenuLabel.SetText("");

        const int localFocus = _choiceSelected - _choiceScroll;

        for (int visRow = 0; visRow < kNumVisibleItems; visRow++)
        {
            int choiceIdx = _choiceScroll + visRow;
            bool rowFocused = (visRow == localFocus);
            int xOffset = rowFocused ? LE_FOCUS_OFFSET_X : 0;

            Label2DView* nameLabel = GetItemNameLabel(visRow);
            Label2DView* valueLabel = GetItemValueLabel(visRow);
            int rowY = _position.y + LE_ITEM_Y0_SUBMENU + visRow * LE_ITEM_YSTEP;

            if (choiceIdx < _choiceCount)
            {
                nameLabel->SetText("");
                valueLabel->SetText(GetChoiceLabel(_choiceKind, choiceIdx));
            }
            else
            {
                nameLabel->SetText("");
                valueLabel->SetText("");
            }

            nameLabel->SetPosition(LE_ITEMNAME_X + xOffset, rowY);
            valueLabel->SetPosition(LE_ITEMVALUE_X + xOffset, rowY);
        }

        _scrollHintLabel.SetText("");
        _scrollHintLabel.SetPosition(LE_HINT_X, _position.y + LE_HINT_Y);
        return;
    }

    _slotLabel.SetText("");
    _saveLabel.SetText("");
    _resetLabel.SetText("");
    _resetMenuLabel.SetText("");
    _subMenuLabel.SetText("");

    int topVisRow = -1;
    if (_itemScrollOffset <= 0)
        topVisRow = -_itemScrollOffset;

    if (topVisRow >= 0 && topVisRow < kNumVisibleItems)
    {
        int rowY = _position.y + LE_ITEM_Y0_SUBMENU + topVisRow * LE_ITEM_YSTEP;
        char slotBuf[16];
        mini_snprintf(slotBuf, sizeof(slotBuf), _focusRow == kFocusSlot ? "[%u]" : "%u", _layoutService->GetCurrentSlot());

        _slotLabel.SetText(slotBuf);
        _saveLabel.SetText(_focusRow == kFocusSave ? "[Save]" : "Save");
        _resetLabel.SetText(_focusRow == kFocusReset ? "[Reset]" : "Reset");
        _resetMenuLabel.SetText(_focusRow == kFocusResetMenu ? "[Reset Menu]" : "Reset Menu");

        _slotLabel.SetPosition(LE_SLOT_X, rowY);
        _saveLabel.SetPosition(LE_SAVE_X, rowY);
        _resetLabel.SetPosition(LE_RESET_X, rowY);
        _resetMenuLabel.SetPosition(LE_RESET_MENU_X, rowY);
    }

    const int itemCount = GetSubMenuItemCount(_currentSubMenu);
    const int totalCount = 2 + itemCount;
    for (int visRow = 0; visRow < kNumVisibleItems; visRow++)
    {
        int rowIdx = _itemScrollOffset + visRow;
        bool rowFocused = false;
        if (rowIdx == 1)
            rowFocused = (_focusRow == kFocusSubMenu);
        else if (rowIdx >= 2)
            rowFocused = (_focusRow == (kFocusItem0 + (rowIdx - 2)));
        int xOffset = rowFocused ? LE_FOCUS_OFFSET_X : 0;

        Label2DView* nameLabel = GetItemNameLabel(visRow);
        Label2DView* valueLabel = GetItemValueLabel(visRow);
        int rowY = _position.y + LE_ITEM_Y0_SUBMENU + visRow * LE_ITEM_YSTEP;
        nameLabel->SetHorizontalAlignment(Alignment::Start);
        valueLabel->SetHorizontalAlignment(Alignment::Start);

        if (rowIdx < totalCount)
        {
            if (rowIdx == 0)
            {
                nameLabel->SetText("");
                valueLabel->SetText("");
            }
            else if (rowIdx == 1)
            {
                nameLabel->SetText("");
                char menuBuf[32];
                mini_snprintf(menuBuf, sizeof(menuBuf), rowFocused ? "[%s]" : "%s", GetSubMenuName(_currentSubMenu));
                valueLabel->SetText(menuBuf);
                valueLabel->SetHorizontalAlignment(Alignment::Start);
                valueLabel->SetPosition(LE_ITEMNAME_X + xOffset, rowY);
                nameLabel->SetPosition(LE_ITEMNAME_X, rowY); 
                continue;
            }
            else
            {
                int itemIdx = rowIdx - 2;
                int indent = GetChildIndentX(_currentSubMenu, itemIdx);
                nameLabel->SetText(GetItemName(_currentSubMenu, itemIdx));
                char vbuf[28];
                GetItemValueText(_currentSubMenu, itemIdx, vbuf, sizeof(vbuf));
                valueLabel->SetText(vbuf);
                nameLabel->SetPosition(LE_ITEMNAME_X + indent + xOffset, rowY);
                valueLabel->SetPosition(LE_ITEMVALUE_X + xOffset, rowY);
                continue;
            }
        }
        else
        {
            nameLabel->SetText("");
            valueLabel->SetText("");
        }

        nameLabel->SetPosition(LE_ITEMNAME_X + xOffset, rowY);
        valueLabel->SetPosition(LE_ITEMVALUE_X + xOffset, rowY);
    }

    _scrollHintLabel.SetPosition(LE_HINT_X, _position.y + LE_HINT_Y);
}

void LayoutEditorBottomSheetView::Update()
{
    BottomSheetView::Update();
    UpdateAllLabels();
}

void LayoutEditorBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto bgColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        auto fgNormal = _materialColorScheme->onSurfaceVariant;
        auto fgTitle = _materialColorScheme->onSurface;
        auto bgFocused = _materialColorScheme->GetColor(md::sys::color::secondaryContainer);
        auto fgFocused = _materialColorScheme->GetColor(md::sys::color::onSecondaryContainer);

        _titleLabel.SetBackgroundColor(bgColor);
        _titleLabel.SetForegroundColor(fgTitle);

        _slotLabel.SetBackgroundColor(_focusRow == kFocusSlot && !_isChoosing ? bgFocused : bgColor);
        _slotLabel.SetForegroundColor(_focusRow == kFocusSlot && !_isChoosing ? fgFocused : fgNormal);

        _saveLabel.SetBackgroundColor(_focusRow == kFocusSave && !_isChoosing ? bgFocused : bgColor);
        _saveLabel.SetForegroundColor(_focusRow == kFocusSave && !_isChoosing ? fgFocused : fgNormal);

        _resetLabel.SetBackgroundColor(_focusRow == kFocusReset && !_isChoosing ? bgFocused : bgColor);
        _resetLabel.SetForegroundColor(_focusRow == kFocusReset && !_isChoosing ? fgFocused : fgNormal);

        _resetMenuLabel.SetBackgroundColor(_focusRow == kFocusResetMenu && !_isChoosing ? bgFocused : bgColor);
        _resetMenuLabel.SetForegroundColor(_focusRow == kFocusResetMenu && !_isChoosing ? fgFocused : fgNormal);

        _subMenuLabel.SetBackgroundColor(_focusRow == kFocusSubMenu && !_isChoosing ? bgFocused : bgColor);
        _subMenuLabel.SetForegroundColor(_focusRow == kFocusSubMenu && !_isChoosing ? fgFocused : fgNormal);

        int count = _isChoosing ? _choiceCount : (2 + GetSubMenuItemCount(_currentSubMenu));
        int localFocus = _isChoosing ? (_choiceSelected - _choiceScroll) : -1;
        auto headerBg = _materialColorScheme->surfaceContainerHighest;
        for (int visRow = 0; visRow < kNumVisibleItems; visRow++)
        {
            int rowIdx = (_isChoosing ? _choiceScroll : _itemScrollOffset) + visRow;
            bool rowFocused = false;
            if (_isChoosing)
                rowFocused = (visRow == localFocus) && (rowIdx < count);
            else
            {
                if (rowIdx == 0)
                    rowFocused = (_focusRow <= kFocusResetMenu) && (rowIdx < count);
                else if (rowIdx == 1)
                    rowFocused = (_focusRow == kFocusSubMenu) && (rowIdx < count);
                else
                    rowFocused = (_focusRow == (kFocusItem0 + (rowIdx - 2))) && (rowIdx < count);
            }

            bool isGroupHeader = !_isChoosing && rowIdx >= 2 && rowIdx < count
                && IsNonFocusableItem(_currentSubMenu, rowIdx - 2);

            Label2DView* nameLabel = GetItemNameLabel(visRow);
            Label2DView* valueLabel = GetItemValueLabel(visRow);

            auto rowBg = rowFocused ? bgFocused : (isGroupHeader ? headerBg : bgColor);
            auto rowFg = rowFocused ? fgFocused : (isGroupHeader ? fgTitle : fgNormal);

            nameLabel->SetBackgroundColor(rowBg);
            nameLabel->SetForegroundColor(rowFg);
            valueLabel->SetBackgroundColor(rowBg);
            valueLabel->SetForegroundColor(rowFg);
        }

        _scrollHintLabel.SetBackgroundColor(bgColor);
        _scrollHintLabel.SetForegroundColor(fgNormal);

        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool LayoutEditorBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& /*focusManager*/)
{
    _holdLeft = inputProvider.Current(InputKey::DpadLeft) ? (u16)std::min<int>(0xFFFF, _holdLeft + 1) : 0;
    _holdRight = inputProvider.Current(InputKey::DpadRight) ? (u16)std::min<int>(0xFFFF, _holdRight + 1) : 0;
    _holdUp = inputProvider.Current(InputKey::DpadUp) ? (u16)std::min<int>(0xFFFF, _holdUp + 1) : 0;
    _holdDown = inputProvider.Current(InputKey::DpadDown) ? (u16)std::min<int>(0xFFFF, _holdDown + 1) : 0;

    bool upPressed = inputProvider.Triggered(InputKey::DpadUp)
        || (_holdUp >= 18 && (_holdUp % 4) == 0);
    bool downPressed = inputProvider.Triggered(InputKey::DpadDown)
        || (_holdDown >= 18 && (_holdDown % 4) == 0);
    bool leftPressed = inputProvider.Triggered(InputKey::DpadLeft)
        || (_holdLeft >= 18 && (_holdLeft % 3) == 0);
    bool rightPressed = inputProvider.Triggered(InputKey::DpadRight)
        || (_holdRight >= 18 && (_holdRight % 3) == 0);

    if (_isChoosing)
    {
        if (upPressed)
        {
            if (_choiceSelected > 0)
                _choiceSelected--;
            if (_choiceSelected < _choiceScroll)
                _choiceScroll = _choiceSelected;
            return true;
        }
        else if (downPressed)
        {
            if (_choiceSelected < _choiceCount - 1)
                _choiceSelected++;
            if (_choiceSelected >= _choiceScroll + kNumVisibleItems)
                _choiceScroll = _choiceSelected - (kNumVisibleItems - 1);
            return true;
        }

        if (inputProvider.Triggered(InputKey::B))
        {
            CancelChoice();
            return true;
        }

        bool selectionChanged = false;

        if (selectionChanged
            && _choiceKind != ChoiceKind::Slot
            && _choiceKind != ChoiceKind::SubMenu)
        {
            ApplyChoiceValue(_choiceKind, _choiceSelected);
            return true;
        }

        if (selectionChanged)
        {
            return true;
        }

        if (inputProvider.Triggered(InputKey::A))
        {
            ConfirmChoice();
            return true;
        }

        return false;
    }

    if (inputProvider.Triggered(InputKey::B))
    {
        _controller->HideLayoutEditor();
        return true;
    }

    if (inputProvider.Triggered(InputKey::L | InputKey::R))
    {
        int delta = inputProvider.Triggered(InputKey::R) ? 1 : -1;
        _currentSubMenu = ClampInt(_currentSubMenu + delta, 0, GetSubMenuCount() - 1);
        _focusRow = kFocusSubMenu;
        ClampItemScroll();
        EnsureItemFocusVisible();
        return true;
    }

    bool consumed = false;
    auto focusToVisualRow = [&](int focusRow) -> int
    {
        if (focusRow <= kFocusResetMenu)
            return 0;
        if (focusRow == kFocusSubMenu)
            return 1;
        return 2 + (focusRow - kFocusItem0);
    };
    auto visualRowToFocus = [&](int visualRow) -> int
    {
        if (visualRow <= 0)
            return _topActionFocus;
        if (visualRow == 1)
            return kFocusSubMenu;
        return kFocusItem0 + (visualRow - 2);
    };
    auto isRowFocusable = [&](int rowIdx) -> bool
    {
        if (rowIdx < 0)
            return false;
        int totalRows = 2 + GetSubMenuItemCount(_currentSubMenu);
        if (rowIdx >= totalRows)
            return false;
        if (rowIdx <= 1)
            return true;
        return !IsNonFocusableItem(_currentSubMenu, rowIdx - 2);
    };

    if (upPressed)
    {
        int currentVisual = focusToVisualRow(_focusRow);
        for (int row = currentVisual - 1; row >= 0; --row)
        {
            if (isRowFocusable(row))
            {
                _focusRow = visualRowToFocus(row);
                EnsureItemFocusVisible();
                break;
            }
        }

        consumed = true;
    }
    else if (downPressed)
    {
        int currentVisual = focusToVisualRow(_focusRow);
        int totalRows = 2 + GetSubMenuItemCount(_currentSubMenu);
        for (int row = currentVisual + 1; row < totalRows; ++row)
        {
            if (isRowFocusable(row))
            {
                _focusRow = visualRowToFocus(row);
                EnsureItemFocusVisible();
                break;
            }
        }

        consumed = true;
    }

    if (leftPressed || rightPressed)
    {
        bool right = rightPressed;
        int step = right ? GetHoldAccelerationStep(_holdRight) : GetHoldAccelerationStep(_holdLeft);
        int delta = right ? step : -step;

        if (_focusRow <= kFocusResetMenu)
        {
            _focusRow = ClampInt(_focusRow + (delta > 0 ? 1 : -1), kFocusSlot, kFocusResetMenu);
            _topActionFocus = _focusRow;
            consumed = true;
        }
        else if (_focusRow == kFocusSubMenu)
        {
            _currentSubMenu = ClampInt(_currentSubMenu + (delta > 0 ? 1 : -1), 0, GetSubMenuCount() - 1);
            ClampItemScroll();
            EnsureItemFocusVisible();
            consumed = true;
        }
        else
        {
            int itemIdx = _focusRow - kFocusItem0;
            int count = GetSubMenuItemCount(_currentSubMenu);
            if (itemIdx < count)
            {
                if (!IsNonFocusableItem(_currentSubMenu, itemIdx))
                {
                    ChangeItemValue(_currentSubMenu, itemIdx, delta);
                    consumed = true;
                }
            }
        }
    }

    if (inputProvider.Triggered(InputKey::A))
    {
        if (_focusRow == kFocusSave)
        {
            _layoutService->SaveCurrentSlot();
            if (_themeColorDirty)
            {
                if (SaveThemeColorToFile())
                {
                    if (_themePreviewApplied && _materialColorScheme != nullptr)
                    {
                        _originalMaterialColorScheme = *const_cast<MaterialColorScheme*>(_materialColorScheme);
                        _themePreviewApplied = false;
                    }
                    _themeColorDirty = false;
                }
            }
            consumed = true;
        }
        else if (_focusRow == kFocusReset)
        {
            _layoutService->ResetCurrentSlot();
            if (_themeColorDirty)
            {
                RestoreThemeColorPreview();
                _themeColorDirty = false;
                _themeColorLoaded = false;
                EnsureThemeColorLoaded();
            }
            consumed = true;
        }
        else if (_focusRow == kFocusResetMenu)
        {
            if (_currentSubMenu == UI_SUBMENU_THEMECOLOR)
            {
                if (_themePreviewApplied)
                    RestoreThemeColorPreview();
                _themeColorDirty = false;
                _themeColorLoaded = false;
                EnsureThemeColorLoaded();
            }
            else
            {
                ResetSubMenuToDefaults(_layoutService->GetCurrentLayoutMutable(), _currentSubMenu);
            }
            consumed = true;
        }
        else
        {
            ChoiceKind kind;
            int count = 0;
            int selectedValue = 0;
            if (GetChoiceListForCurrentFocus(kind, count, selectedValue))
            {
                int targetIdx = -1;
                if (_focusRow >= kFocusItem0)
                    targetIdx = _focusRow - kFocusItem0;
                BeginChoice(kind, count, selectedValue, targetIdx);
                consumed = true;
            }
            else if (_focusRow >= kFocusItem0)
            {
                int itemIdx = _focusRow - kFocusItem0;
                int countItems = GetSubMenuItemCount(_currentSubMenu);
                if (itemIdx < countItems && !IsNonFocusableItem(_currentSubMenu, itemIdx))
                {
                    bool isToggle = false;
                    switch (_currentSubMenu)
                    {
                        case UI_SUBMENU_DATETIME1:
                        case UI_SUBMENU_DATETIME2:
                            isToggle = (itemIdx == 0);
                            break;
                        case UI_SUBMENU_PREFIX:
                            isToggle = (itemIdx == 0 || itemIdx == 4);
                            break;
                        case UI_SUBMENU_GAME_ID:
                            isToggle = (itemIdx == 0 || itemIdx == 4 || itemIdx == 5);
                            break;
                        case UI_SUBMENU_REGION:
                            isToggle = (itemIdx == 0 || itemIdx == 4);
                            break;
                        case UI_SUBMENU_VERSION:
                        case UI_SUBMENU_CRC:
                        case UI_SUBMENU_USERNAME:
                            isToggle = (itemIdx == 0);
                            break;
                        case UI_SUBMENU_BOXART:
                        case UI_SUBMENU_ICON:
                            isToggle = (itemIdx == 0);
                            break;
                        case UI_SUBMENU_ROMNAME:
                            isToggle = (itemIdx == 1);
                            break;
                        case UI_SUBMENU_FILENAME:
                            isToggle = (itemIdx == 0 || itemIdx == 4);
                            break;
                        case UI_SUBMENU_THEMECOLOR:
                            isToggle = (itemIdx == 4);
                            break;
                        default:
                            break;
                    }
                    if (isToggle)
                    {
                        if (_currentSubMenu == UI_SUBMENU_THEMECOLOR && itemIdx == 4)
                        {
                            EnsureThemeColorLoaded();
                            ApplyThemeColorPreview();
                        }
                        else
                        {
                            ChangeItemValue(_currentSubMenu, itemIdx, 1);
                        }
                        consumed = true;
                    }
                }
            }
        }
    }

    return consumed;
}
