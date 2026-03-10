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
#define LE_SUBMENU_X    15
#define LE_ITEMNAME_X   20
#define LE_ITEMVALUE_X  120
#define LE_HINT_X       15

#define LE_FOCUS_OFFSET_X 6

// submenus.
#define UI_SUBMENU_DATETIME1  0
#define UI_SUBMENU_DATETIME2  1
#define UI_SUBMENU_ROM_IDCODE 2
#define UI_SUBMENU_BOXART     3
#define UI_SUBMENU_ICON       4
#define UI_SUBMENU_ROMNAME    5
#define UI_SUBMENU_FILENAME   6
#define UI_SUBMENU_THEMECOLOR 7
#define UI_SUBMENU_COUNT      8

static const char* const kUiSubMenuNames[UI_SUBMENU_COUNT] = {
    "DateTime1",
    "DateTime2",
    "ROM IdCode",
    "Box Art",
    "Icon",
    "ROM Name",
    "File Name",
    "Theme Color",
};

static const char* const kItemNamesDateTime[] = {
    "Visible", "Y", "X", "Format", "Separator", "Font"
};
static const char* const kItemNamesElement[] = {
    "Visible", "Y", "X", "Font"
};
static const char* const kItemNamesNoFont[] = {
    "Visible", "Y", "X"
};
static const char* const kItemNamesFileName[] = {
    "Visible", "Y", "X", "Font", "Scroll", "Speed"
};
static const char* const kItemNamesRomName[] = {
    "Line N", "Visible", "X", "Y", "Font"
};
static const char* const kItemNamesThemeColor[] = {
    "R", "G", "B", "Apply now"
};

static const u8 kSubMenuItemCounts[UI_SUBMENU_COUNT] = {
    6,
    6,
    4,
    3,
    3,
    5,
    6,
    4,
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
}

void LayoutEditorBottomSheetView::RestoreThemeColorPreview()
{
    if (!_themePreviewApplied || _materialColorScheme == nullptr)
        return;

    *const_cast<MaterialColorScheme*>(_materialColorScheme) = _originalMaterialColorScheme;
    _themePreviewApplied = false;
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
    if (_focusRow < kFocusItem0)
        return;

    int visRow = _focusRow - kFocusItem0;
    int itemIdx = _itemScrollOffset + visRow;
    int count = GetSubMenuItemCount(_currentSubMenu);
    if (count <= 0)
    {
        _focusRow = kFocusSubMenu;
        _itemScrollOffset = 0;
        return;
    }

    if (itemIdx < 0)
        itemIdx = 0;
    if (itemIdx >= count)
        itemIdx = count - 1;

    if (itemIdx < _itemScrollOffset)
        _itemScrollOffset = itemIdx;
    else if (itemIdx >= _itemScrollOffset + kNumVisibleItems)
        _itemScrollOffset = itemIdx - (kNumVisibleItems - 1);

    ClampItemScroll();

    _focusRow = kFocusItem0 + (itemIdx - _itemScrollOffset);
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
    if (_themeColorDirty)
    {
        RestoreThemeColorPreview();
        _themeColorDirty = false;
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
            return (u32)itemIdx < 6 ? kItemNamesDateTime[itemIdx] : "";
        case UI_SUBMENU_ROM_IDCODE:
            return (u32)itemIdx < 4 ? kItemNamesElement[itemIdx] : "";
        case UI_SUBMENU_BOXART:
        case UI_SUBMENU_ICON:
            return (u32)itemIdx < 3 ? kItemNamesNoFont[itemIdx] : "";
        case UI_SUBMENU_ROMNAME:
            return (u32)itemIdx < 5 ? kItemNamesRomName[itemIdx] : "";
        case UI_SUBMENU_FILENAME:
            return (u32)itemIdx < 6 ? kItemNamesFileName[itemIdx] : "";
        case UI_SUBMENU_THEMECOLOR:
            return (u32)itemIdx < 4 ? kItemNamesThemeColor[itemIdx] : "";
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
            }
            break;
        case UI_SUBMENU_ROM_IDCODE:
            switch (itemIdx)
            {
                case 0: WRITE_VISIBLE(d.romIdCode.visible); break;
                case 1: WRITE_COORD(d.romIdCode.y); break;
                case 2: WRITE_COORD(d.romIdCode.x); break;
                case 3: WRITE_FONT(d.romIdCode.font); break;
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
                case 0: mini_snprintf(buf, bufLen, "Line %d", _romNameSelectedLine + 1); break;
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
            }
            break;
        case UI_SUBMENU_THEMECOLOR:
            EnsureThemeColorLoaded();
            switch (itemIdx)
            {
                case 0: mini_snprintf(buf, bufLen, "%d", _themeColorR); break;
                case 1: mini_snprintf(buf, bufLen, "%d", _themeColorG); break;
                case 2: mini_snprintf(buf, bufLen, "%d", _themeColorB); break;
                case 3: mini_snprintf(buf, bufLen, "Press A"); break;
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
            }
            break;
        case UI_SUBMENU_ROM_IDCODE:
            switch (itemIdx)
            {
                case 0: toggleU8(d.romIdCode.visible); break;
                case 1: changeY(d.romIdCode.y); break;
                case 2: changeX(d.romIdCode.x); break;
                case 3: cycleU8(d.romIdCode.font, delta, LAYOUT_FONT_COUNT); break;
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
            }
            break;
        case UI_SUBMENU_THEMECOLOR:
            EnsureThemeColorLoaded();
            if (itemIdx == 0) _themeColorR = WrapRange(_themeColorR + delta, 0, 255);
            if (itemIdx == 1) _themeColorG = WrapRange(_themeColorG + delta, 0, 255);
            if (itemIdx == 2) _themeColorB = WrapRange(_themeColorB + delta, 0, 255);
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

    int visRow = _focusRow - kFocusItem0;
    int itemIdx = _itemScrollOffset + visRow;
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
        case UI_SUBMENU_ROM_IDCODE:
            if (itemIdx == 3) { kind = ChoiceKind::Font; count = LAYOUT_FONT_COUNT; selectedValue = d.romIdCode.font; return true; }
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
        _itemScrollOffset = 0;
        ClampItemScroll();
        return;
    }

    if (kind == ChoiceKind::SubMenu)
    {
        _currentSubMenu = valueIdx;
        _itemScrollOffset = 0;
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
        case UI_SUBMENU_ROM_IDCODE:
            if (_choiceTargetIdx == 3 && kind == ChoiceKind::Font) d.romIdCode.font = (u8)valueIdx;
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
    int count = GetSubMenuItemCount(_currentSubMenu);
    int maxScroll = count - kNumVisibleItems;
    if (maxScroll < 0) maxScroll = 0;
    if (_itemScrollOffset < 0) _itemScrollOffset = 0;
    if (_itemScrollOffset > maxScroll) _itemScrollOffset = maxScroll;
}

void LayoutEditorBottomSheetView::UpdateAllLabels()
{
    EnsureItemFocusVisible();

    _titleLabel.SetText("Layout Editor");
    _titleLabel.SetPosition(LE_TITLE_X, _position.y + LE_TITLE_Y);

    {
        char buf[16];
        if (_focusRow == kFocusSlot)
            mini_snprintf(buf, sizeof(buf), "[%u]", _layoutService->GetCurrentSlot());
        else
            mini_snprintf(buf, sizeof(buf), "%u", _layoutService->GetCurrentSlot());
        _slotLabel.SetText(buf);
        _slotLabel.SetPosition(LE_SLOT_X, _position.y + LE_SLOT_Y);
    }

    _saveLabel.SetText(_focusRow == kFocusSave ? "[Save]" : "Save");
    _resetLabel.SetText(_focusRow == kFocusReset ? "[Reset]" : "Reset");
    _saveLabel.SetPosition(LE_SAVE_X, _position.y + LE_SAVERES_Y);
    _resetLabel.SetPosition(LE_RESET_X, _position.y + LE_SAVERES_Y);

    {
        char buf[28];
        if (_focusRow == kFocusSubMenu)
            mini_snprintf(buf, sizeof(buf), "[%s]", GetSubMenuName(_currentSubMenu));
        else
            mini_snprintf(buf, sizeof(buf), "%s", GetSubMenuName(_currentSubMenu));
        _subMenuLabel.SetText(buf);
        _subMenuLabel.SetPosition(LE_SUBMENU_X, _position.y + LE_SUBMENU_Y);
    }

    if (_isChoosing)
    {
        switch (_choiceKind)
        {
            case ChoiceKind::Slot: _titleLabel.SetText("Select Slot"); break;
            case ChoiceKind::SubMenu: _titleLabel.SetText("Select Menu"); break;
            case ChoiceKind::Format: _titleLabel.SetText("Select Format"); break;
            case ChoiceKind::Separator: _titleLabel.SetText("Select Separator"); break;
            case ChoiceKind::Font: _titleLabel.SetText("Select Font"); break;
            default: _titleLabel.SetText("Select Value"); break;
        }

        _slotLabel.SetText("");
        _saveLabel.SetText("");
        _resetLabel.SetText("");
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

    int count = GetSubMenuItemCount(_currentSubMenu);
    for (int visRow = 0; visRow < kNumVisibleItems; visRow++)
    {
        int itemIdx = _itemScrollOffset + visRow;
        bool rowFocused = (_focusRow == kFocusItem0 + visRow) && (itemIdx < count)
            && !IsNonFocusableItem(_currentSubMenu, itemIdx);
        int xOffset = rowFocused ? LE_FOCUS_OFFSET_X : 0;
        int indent = GetChildIndentX(_currentSubMenu, itemIdx);

        Label2DView* nameLabel = GetItemNameLabel(visRow);
        Label2DView* valueLabel = GetItemValueLabel(visRow);
        int rowY = _position.y + LE_ITEM_Y0 + visRow * LE_ITEM_YSTEP;

        if (itemIdx < count)
        {
            nameLabel->SetText(GetItemName(_currentSubMenu, itemIdx));
            char vbuf[28];
            GetItemValueText(_currentSubMenu, itemIdx, vbuf, sizeof(vbuf));
            valueLabel->SetText(vbuf);
        }
        else
        {
            nameLabel->SetText("");
            valueLabel->SetText("");
        }

        nameLabel->SetPosition(LE_ITEMNAME_X + indent + xOffset, rowY);
        valueLabel->SetPosition(LE_ITEMVALUE_X + xOffset, rowY);
    }

    _scrollHintLabel.SetText("");
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

        _subMenuLabel.SetBackgroundColor(_focusRow == kFocusSubMenu && !_isChoosing ? bgFocused : bgColor);
        _subMenuLabel.SetForegroundColor(_focusRow == kFocusSubMenu && !_isChoosing ? fgFocused : fgNormal);

        int count = _isChoosing ? _choiceCount : GetSubMenuItemCount(_currentSubMenu);
        int localFocus = _isChoosing ? (_choiceSelected - _choiceScroll) : -1;
        auto headerBg = _materialColorScheme->surfaceContainerHighest;
        for (int visRow = 0; visRow < kNumVisibleItems; visRow++)
        {
            int itemIdx = (_isChoosing ? _choiceScroll : _itemScrollOffset) + visRow;
            bool rowFocused = false;
            if (_isChoosing)
                rowFocused = (visRow == localFocus) && (itemIdx < count);
            else
                rowFocused = (_focusRow == kFocusItem0 + visRow) && (itemIdx < count)
                    && !IsNonFocusableItem(_currentSubMenu, itemIdx);

            bool isGroupHeader = !_isChoosing && itemIdx < count
                && IsNonFocusableItem(_currentSubMenu, itemIdx);

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

    if (_isChoosing)
    {
        if (inputProvider.Triggered(InputKey::DpadUp))
        {
            if (_choiceSelected > 0)
                _choiceSelected--;
            if (_choiceSelected < _choiceScroll)
                _choiceScroll = _choiceSelected;
            return true;
        }
        else if (inputProvider.Triggered(InputKey::DpadDown))
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
        _currentSubMenu = (_currentSubMenu + delta + GetSubMenuCount()) % GetSubMenuCount();
        _itemScrollOffset = 0;
        _focusRow = kFocusSubMenu;
        ClampItemScroll();
        return true;
    }

    bool consumed = false;
    auto focusItem = [&](int itemIdx)
    {
        if (itemIdx < 0)
        {
            _focusRow = kFocusSubMenu;
            return;
        }

        if (itemIdx < _itemScrollOffset)
            _itemScrollOffset = itemIdx;
        else if (itemIdx >= _itemScrollOffset + kNumVisibleItems)
            _itemScrollOffset = itemIdx - (kNumVisibleItems - 1);

        ClampItemScroll();
        _focusRow = kFocusItem0 + (itemIdx - _itemScrollOffset);
        EnsureItemFocusVisible();
    };

    if (inputProvider.Triggered(InputKey::DpadUp))
    {
        int count = GetSubMenuItemCount(_currentSubMenu);

        if (_focusRow >= kFocusItem0)
        {
            int currentIdx = _itemScrollOffset + (_focusRow - kFocusItem0);
            int nextIdx = GetNextFocusableItem(_currentSubMenu, currentIdx, -1);
            focusItem(nextIdx);
        }
        else if (_focusRow == kFocusSubMenu)
        {
            _focusRow = kFocusSlot;
        }
        else
        {
            if (count > 0)
            {
                int nextIdx = GetNextFocusableItem(_currentSubMenu, count, -1);
                focusItem(nextIdx);
            }
            else
            {
                _focusRow = kFocusSubMenu;
            }
        }

        consumed = true;
    }
    else if (inputProvider.Triggered(InputKey::DpadDown))
    {
        int count = GetSubMenuItemCount(_currentSubMenu);

        if (_focusRow == kFocusSlot || _focusRow == kFocusSave || _focusRow == kFocusReset)
        {
            _focusRow = kFocusSubMenu;
        }
        else if (_focusRow == kFocusSubMenu)
        {
            if (count > 0)
            {
                int nextIdx = GetNextFocusableItem(_currentSubMenu, -1, 1);
                focusItem(nextIdx);
            }
        }
        else
        {
            int currentIdx = _itemScrollOffset + (_focusRow - kFocusItem0);
            int nextIdx = GetNextFocusableItem(_currentSubMenu, currentIdx, 1);
            if (nextIdx >= 0)
                focusItem(nextIdx);
        }

        consumed = true;
    }

    if (inputProvider.Triggered(InputKey::DpadLeft) || inputProvider.Triggered(InputKey::DpadRight))
    {
        bool right = inputProvider.Triggered(InputKey::DpadRight);
        int step = right ? GetHoldAccelerationStep(_holdRight) : GetHoldAccelerationStep(_holdLeft);
        int delta = right ? step : -step;

        if (_focusRow == kFocusSlot)
        {
            _focusRow = (delta > 0) ? kFocusSave : kFocusReset;
            consumed = true;
        }
        else if (_focusRow == kFocusSave)
        {
            _focusRow = (delta > 0) ? kFocusReset : kFocusSlot;
            consumed = true;
        }
        else if (_focusRow == kFocusReset)
        {
            _focusRow = (delta > 0) ? kFocusSlot : kFocusSave;
            consumed = true;
        }
        else if (_focusRow == kFocusSubMenu)
        {
            _currentSubMenu = (_currentSubMenu + delta + GetSubMenuCount()) % GetSubMenuCount();
            _itemScrollOffset = 0;
            ClampItemScroll();
            consumed = true;
        }
        else
        {
            int visRow = _focusRow - kFocusItem0;
            int itemIdx = _itemScrollOffset + visRow;
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
                SaveThemeColorToFile();
                _themeColorDirty = false;
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
        else
        {
            ChoiceKind kind;
            int count = 0;
            int selectedValue = 0;
            if (GetChoiceListForCurrentFocus(kind, count, selectedValue))
            {
                int targetIdx = -1;
                if (_focusRow >= kFocusItem0)
                {
                    int visRow = _focusRow - kFocusItem0;
                    targetIdx = _itemScrollOffset + visRow;
                }
                BeginChoice(kind, count, selectedValue, targetIdx);
                consumed = true;
            }
            else if (_focusRow >= kFocusItem0)
            {
                int visRow = _focusRow - kFocusItem0;
                int itemIdx = _itemScrollOffset + visRow;
                int countItems = GetSubMenuItemCount(_currentSubMenu);
                if (itemIdx < countItems && !IsNonFocusableItem(_currentSubMenu, itemIdx))
                {
                    bool isToggle = false;
                    switch (_currentSubMenu)
                    {
                        case UI_SUBMENU_DATETIME1:
                        case UI_SUBMENU_DATETIME2:
                        case UI_SUBMENU_ROM_IDCODE:
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
                            isToggle = (itemIdx == 3);
                            break;
                        default:
                            break;
                    }
                    if (isToggle)
                    {
                        if (_currentSubMenu == UI_SUBMENU_THEMECOLOR && itemIdx == 3)
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
