#include "common.h"
#include <memory>
#include <string.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include "json/ArduinoJson.h"
#include "core/mini-printf.h"
#include "CustomTopBackgroundType.h"
#include "gui/VramContext.h"
#include "../material/MaterialColorSchemeFactory.h"
#include "romBrowser/views/IconButton3DView.h"
#include "CustomTheme.h"

#define JSON_RESERVED_SIZE  4096

#define KEY_COLOR_R     "r"
#define KEY_COLOR_G     "g"
#define KEY_COLOR_B     "b"

#define KEY_POINT_X     "x"
#define KEY_POINT_Y     "y"

#define KEY_TOP_ICON                    "topIcon"
#define KEY_TOP_BANNER_TEXT_LINE_0      "topBannerTextLine0"
#define KEY_TOP_BANNER_TEXT_LINE_1      "topBannerTextLine1"
#define KEY_TOP_BANNER_TEXT_LINE_2      "topBannerTextLine2"
#define KEY_TOP_FILE_NAME_TEXT          "topFileNameText"
#define KEY_TOP_COVER                   "topCover"
#define KEY_GRID_ICON                   "gridIcon"
#define KEY_BANNER_LIST_ICON            "bannerListIcon"
#define KEY_BANNER_LIST_TEXT_LINE_0     "bannerListTextLine0"
#define KEY_BANNER_LIST_TEXT_LINE_1     "bannerListTextLine1"
#define KEY_BANNER_LIST_TEXT_LINE_2     "bannerListTextLine2"

#define KEY_ELEMENT_POSITION        "position"
#define KEY_ELEMENT_WIDTH           "width"
#define KEY_ELEMENT_TEXT_COLOR      "textColor"
#define KEY_ELEMENT_BLEND_COLOR     "blendColor"
#define KEY_ELEMENT_FONT            "font"
#define KEY_ELEMENT_VISIBLE         "visible"
#define KEY_ELEMENT_MARQUEE         "marquee"

static const CustomThemeInfo sDefaultCustomThemeInfo
{
    .topIconInfo = CustomTopIconInfo(Point(24, 132), Rgb8(200, 200, 200)),
    .topBannerTextLine0Info = CustomTopTextElementInfo(Point(70, 126), 176, Rgb8(30, 30, 30), Rgb8(200, 200, 200), FontType::Medium11),
    .topBannerTextLine1Info = CustomTopTextElementInfo(Point(70, 141), 176, Rgb8(30, 30, 30), Rgb8(200, 200, 200), FontType::Regular10),
    .topBannerTextLine2Info = CustomTopTextElementInfo(Point(70, 155), 176, Rgb8(30, 30, 30), Rgb8(200, 200, 200), FontType::Regular10),
    .topFileNameTextInfo = CustomTopTextElementInfo(Point(18, 170), 220, Rgb8(30, 30, 30), Rgb8(200, 200, 200), FontType::Medium7_5),
    .topCoverInfo = CustomTopCoverInfo(Point(75, 18)),

    .gridIconInfo = CustomBottomIconInfo(Rgb8(200, 200, 200)),

    .bannerListIconInfo = CustomBottomIconInfo(Rgb8(200, 200, 200)),
    .bannerListTextLine0Info = CustomBannerListTextElementInfo(Rgb8(30, 30, 30), FontType::Medium10),
    .bannerListTextLine1Info = CustomBannerListTextElementInfo(Rgb8(30, 30, 30), FontType::Regular10),
    .bannerListTextLine2Info = CustomBannerListTextElementInfo(Rgb8(30, 30, 30), FontType::Regular10)
};

static CustomTopBackgroundType parseTopBackgroundType(const char* topBackgroundTypeString)
{
    return CustomTopBackgroundType::Bitmap;
}

static Rgb8 parseColor(const JsonObjectConst& json, const Rgb8& defaultColor)
{
    if (json.isNull())
    {
        return defaultColor;
    }

    return Rgb8(
        json[KEY_COLOR_R] | 0,
        json[KEY_COLOR_G] | 0,
        json[KEY_COLOR_B] | 0
    );
}

static Point parsePoint(const JsonObjectConst& json, const Point& defaultPoint)
{
    if (json.isNull())
    {
        return defaultPoint;
    }

    return Point(
        json[KEY_POINT_X] | 0,
        json[KEY_POINT_Y] | 0
    );
}

static FontType parseFontType(const char* fontString, FontType defaultFontType)
{
    if (fontString == nullptr)
        return defaultFontType;

    if (strcmp(fontString, "regular10") == 0)
        return FontType::Regular10;
    if (strcmp(fontString, "medium7_5") == 0)
        return FontType::Medium7_5;
    if (strcmp(fontString, "medium10") == 0)
        return FontType::Medium10;
    if (strcmp(fontString, "medium11") == 0)
        return FontType::Medium11;

    return defaultFontType;
}

static CustomBannerListTextElementInfo parseCustomBannerListTextElementInfo(
    const JsonObjectConst& json, const CustomBannerListTextElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomBannerListTextElementInfo(
        parseColor(json[KEY_ELEMENT_TEXT_COLOR], defaultInfo.GetTextColor()),
        parseFontType(json[KEY_ELEMENT_FONT], defaultInfo.GetFontType()),
        json[KEY_ELEMENT_VISIBLE] | defaultInfo.IsVisible()
    );
}

static CustomBottomIconInfo parseCustomBottomIconInfo(const JsonObjectConst& json, const CustomBottomIconInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomBottomIconInfo(
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor()),
        json[KEY_ELEMENT_VISIBLE] | defaultInfo.IsVisible()
    );
}

static CustomTopIconInfo parseCustomTopIconInfo(const JsonObjectConst& json, const CustomTopIconInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopIconInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor()),
        json[KEY_ELEMENT_VISIBLE] | defaultInfo.IsVisible()
    );
}

static CustomTopCoverInfo parseCustomTopCoverInfo(const JsonObjectConst& json, const CustomTopCoverInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopCoverInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        json[KEY_ELEMENT_VISIBLE] | defaultInfo.IsVisible()
    );
}

static CustomTopTextElementInfo parseCustomTextElementInfo(
    const JsonObjectConst& json, const CustomTopTextElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopTextElementInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        json[KEY_ELEMENT_WIDTH] | defaultInfo.GetWidth(),
        parseColor(json[KEY_ELEMENT_TEXT_COLOR], defaultInfo.GetTextColor()),
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor()),
        parseFontType(json[KEY_ELEMENT_FONT], defaultInfo.GetFontType()),
        json[KEY_ELEMENT_VISIBLE] | defaultInfo.IsVisible(),
        json[KEY_ELEMENT_MARQUEE] | defaultInfo.IsMarqueeEnabled()
    );
}

static CustomThemeInfo parseCustomThemeInfo(const JsonDocument& json)
{
    return CustomThemeInfo
    {
        .topIconInfo = parseCustomTopIconInfo(json[KEY_TOP_ICON], sDefaultCustomThemeInfo.topIconInfo),
        .topBannerTextLine0Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_0], sDefaultCustomThemeInfo.topBannerTextLine0Info),
        .topBannerTextLine1Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_1], sDefaultCustomThemeInfo.topBannerTextLine1Info),
        .topBannerTextLine2Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_2], sDefaultCustomThemeInfo.topBannerTextLine2Info),
        .topFileNameTextInfo = parseCustomTextElementInfo(
            json[KEY_TOP_FILE_NAME_TEXT], sDefaultCustomThemeInfo.topFileNameTextInfo),
        .topCoverInfo = parseCustomTopCoverInfo(json[KEY_TOP_COVER], sDefaultCustomThemeInfo.topCoverInfo),

        .gridIconInfo = parseCustomBottomIconInfo(json[KEY_GRID_ICON], sDefaultCustomThemeInfo.gridIconInfo),

        .bannerListIconInfo = parseCustomBottomIconInfo(json[KEY_BANNER_LIST_ICON], sDefaultCustomThemeInfo.bannerListIconInfo),
        .bannerListTextLine0Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_0], sDefaultCustomThemeInfo.bannerListTextLine0Info),
        .bannerListTextLine1Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_1], sDefaultCustomThemeInfo.bannerListTextLine1Info),
        .bannerListTextLine2Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_2], sDefaultCustomThemeInfo.bannerListTextLine2Info)
    };
}

CustomTheme::CustomTheme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode)
    : Theme(folderName, primaryColor, darkMode)
    , _customThemeInfo(sDefaultCustomThemeInfo)
    , _romBrowserViewFactory(&_customThemeInfo, &_materialColorScheme, &_fontRepository) { }

void CustomTheme::LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext)
{
    const auto file = std::make_unique<File>();
    OpenThemeFile(*file, "theme.json");

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
        return;

    _topBackgroundType = parseTopBackgroundType(json["topBackgroundType"].as<const char*>());
    _customThemeInfo = parseCustomThemeInfo(json);

    mem_setVramDMapping(MEM_VRAM_D_LCDC);
    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    IconButton3DView::UploadGraphics(mainVramContext);
    mem_setVramDMapping(MEM_VRAM_D_TEX_SLOT_0);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);
    _romBrowserViewFactory.LoadResources(*this, mainVramContext);
}
