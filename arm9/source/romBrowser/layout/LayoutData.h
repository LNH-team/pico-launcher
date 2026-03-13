#pragma once
#include "common.h"

// File path template: /_pico/extras/layouts/layoutN.bin
#define LAYOUT_DIR_PATH         "/_pico/extras/layouts"
#define LAYOUT_FILE_PATH_FMT    "/_pico/extras/layouts/layout%u.bin"
#define LAYOUT_FILE_MAGIC       "LYOT"
#define LAYOUT_FILE_VERSION     1

// DateTime formats
// Date-only formats
#define LAYOUT_FMT_DD_MM_YYYY   0
#define LAYOUT_FMT_YYYY_MM_DD   1
#define LAYOUT_FMT_MM_DD_YYYY   2
#define LAYOUT_FMT_YY_MM_DD     3
// Time-only formats
#define LAYOUT_FMT_HH_mm        4
#define LAYOUT_FMT_HH_mm_ss     5
#define LAYOUT_FMT_hh_mm        6
#define LAYOUT_FMT_hh_mm_ss     7
// Date + Time formats
#define LAYOUT_FMT_YYYY_MM_DD_HH_mm     8
#define LAYOUT_FMT_YYYY_MM_DD_HH_mm_ss  9
#define LAYOUT_FMT_DD_MM_YYYY_HH_mm     10
#define LAYOUT_FMT_DD_MM_YYYY_HH_mm_ss  11
#define LAYOUT_FMT_YY_MM_DD_HH_mm       12
#define LAYOUT_FMT_YY_MM_DD_HH_mm_ss    13
#define LAYOUT_FORMAT_COUNT             14

static const char* const kLayoutFormatNames[LAYOUT_FORMAT_COUNT] = {
    "DD MM YYYY",
    "YYYY MM DD",
    "MM DD YYYY",
    "YY MM DD",
    "HH mm",
    "HH mm SS",
    "hh mm",
    "hh mm SS",
    "YYYY MM DD HH mm",
    "YYYY MM DD HH mm SS",
    "DD MM YYYY HH mm",
    "DD MM YYYY HH mm SS",
    "YY MM DD HH mm",
    "YY MM DD HH mm SS",
};

// Separator indices
#define LAYOUT_SEP_DASH     0   // -
#define LAYOUT_SEP_SLASH    1   // /
#define LAYOUT_SEP_COLON    2   // :
#define LAYOUT_SEP_DOT      3   // .
#define LAYOUT_SEP_COMMA    4   // ,
#define LAYOUT_SEP_UNDER    5   // _
#define LAYOUT_SEP_SPACE    6   // (space)
#define LAYOUT_SEP_COUNT    7

static const char* const kLayoutSeparatorNames[LAYOUT_SEP_COUNT] = {
    "-", "/", ":", ".", ",", "_", "Space"
};

static const char kLayoutSeparatorChars[LAYOUT_SEP_COUNT] = {
    '-', '/', ':', '.', ',', '_', ' '
};

// Font indices
#define LAYOUT_FONT_REGULAR10   0
#define LAYOUT_FONT_MEDIUM7_5   1
#define LAYOUT_FONT_MEDIUM10    2
#define LAYOUT_FONT_MEDIUM11    3
#define LAYOUT_FONT_COUNT       4

static const char* const kLayoutFontNames[LAYOUT_FONT_COUNT] = {
    "Regular10", "Medium7.5", "Medium10", "Medium11"
};

// Prefix display mode by ROM
// .GBA
#define LAYOUT_PREFIX_GBA_NATIVE   0
#define LAYOUT_PREFIX_GBA_GBA      1
#define LAYOUT_PREFIX_GBA_COUNT    2

static const char* const kLayoutPrefixGbaModeNames[LAYOUT_PREFIX_GBA_COUNT] = {
    "AGB",
    "GBA",
};

// .NTR
#define LAYOUT_PREFIX_NTR_NATIVE       0
#define LAYOUT_PREFIX_NTR_DS           1
#define LAYOUT_PREFIX_NTR_NATIVE_PLUS  2
#define LAYOUT_PREFIX_NTR_COUNT        3

static const char* const kLayoutPrefixNtrModeNames[LAYOUT_PREFIX_NTR_COUNT] = {
    "NTR",
    "DS",
    "NTR DS",
};

// .TWL
#define LAYOUT_PREFIX_TWL_NATIVE         0
#define LAYOUT_PREFIX_TWL_DSI_ENHANCED   1
#define LAYOUT_PREFIX_TWL_DSI            2
#define LAYOUT_PREFIX_TWL_NATIVE_DSI     3
#define LAYOUT_PREFIX_TWL_NATIVE_ENH     4
#define LAYOUT_PREFIX_TWL_COUNT          5

static const char* const kLayoutPrefixTwlModeNames[LAYOUT_PREFIX_TWL_COUNT] = {
    "TWL",
    "DSi Enhanced",
    "DSi",
    "TWL DSi",
    "TWL DSi Enhanced",
};

// Sub-menu indices
#define LAYOUT_SUBMENU_DATETIME1     0
#define LAYOUT_SUBMENU_DATETIME2     1
#define LAYOUT_SUBMENU_USERNAME      2
#define LAYOUT_SUBMENU_GAME_TITLE    3
#define LAYOUT_SUBMENU_PREFIX        4
#define LAYOUT_SUBMENU_GAME_ID       5
#define LAYOUT_SUBMENU_REGION        6
#define LAYOUT_SUBMENU_CRC           7
#define LAYOUT_SUBMENU_VERSION       8
#define LAYOUT_SUBMENU_BOXART        9
#define LAYOUT_SUBMENU_ICON          10
#define LAYOUT_SUBMENU_ROMNAME       11
#define LAYOUT_SUBMENU_FILENAME      12
#define LAYOUT_SUBMENU_THEME_COLOR   13
#define LAYOUT_SUBMENU_COUNT         14

static const char* const kLayoutSubMenuNames[LAYOUT_SUBMENU_COUNT] = {
    "DateTime1",
    "DateTime2",
    "Username",
    "Game Title",
    "Prefix",
    "Title ID",
    "Region",
    "CRC",
    "Version",
    "Box Art",
    "Icon",
    "ROM Name",
    "File Name",
    "Theme Color",
};

struct LayoutDateTime {
    u8  visible;
    s16 x;
    s16 y;
    u8  format;
    u8  separator;
    u8  font;
    u8  colorR;
    u8  colorG;
    u8  colorB;
};

struct LayoutElement {
    u8  visible;
    s16 x;
    s16 y;
    u8  font;
    u8  colorR;
    u8  colorG;
    u8  colorB;
};

struct LayoutElementNoFont {
    u8  visible;
    s16 x;
    s16 y;
};

struct LayoutPrefixElement {
    u8  visible;
    s16 x;
    s16 y;
    u8  font;
    u8  trailingDash;
    u8  colorR;
    u8  colorG;
    u8  colorB;
    u8  gbaPrefixMode;
    u8  ntrPrefixMode;
    u8  twlPrefixMode;
};

struct LayoutTitleIDElement {
    u8  visible;
    s16 x;
    s16 y;
    u8  font;
    u8  trailingDash;
    u8  colorR;
    u8  colorG;
    u8  colorB;
    u8  showLabelText;
    u8  labelFont;
    s16 labelX;     
    s16 labelY;
    u8  labelColorR;
    u8  labelColorG;
    u8  labelColorB;
};

struct LayoutRegionElement {
    u8  visible;
    s16 x;
    s16 y;
    u8  font;
    u8  trailingDash;
    u8  colorR;
    u8  colorG;
    u8  colorB;
};

struct LayoutFilename {
    u8  visible;
    s16 x;
    s16 y;
    u8  font;
    u8  scroll;
    u8  scrollSpeed;
    u8  colorR;
    u8  colorG;
    u8  colorB;
};

// Full layout data
struct LayoutData {
    LayoutDateTime          dateTime1;
    LayoutDateTime          dateTime2;
    LayoutElement           username;
    LayoutElement           gameTitle;
    LayoutPrefixElement     prefix;
    LayoutTitleIDElement    TitleID;
    LayoutRegionElement     region;
    LayoutElement           crc;
    LayoutElement           version;
    LayoutElementNoFont     boxArt;
    LayoutElementNoFont     icon;
    LayoutElement           romNameRow1;
    LayoutElement           romNameRow2;
    LayoutElement           romNameRow3;
    LayoutFilename          fileName;
};

inline LayoutData LayoutData_Default()
{
    LayoutData d;
    // DateTime1
    d.dateTime1.visible   = 0;
    d.dateTime1.x         = 5;
    d.dateTime1.y         = 2;
    d.dateTime1.format    = LAYOUT_FMT_DD_MM_YYYY;
    d.dateTime1.separator = LAYOUT_SEP_SLASH;
    d.dateTime1.font      = LAYOUT_FONT_REGULAR10;
    d.dateTime1.colorR    = 255;
    d.dateTime1.colorG    = 255;
    d.dateTime1.colorB    = 255;
    // DateTime2
    d.dateTime2.visible   = 0;
    d.dateTime2.x         = 5;
    d.dateTime2.y         = 14;
    d.dateTime2.format    = LAYOUT_FMT_HH_mm_ss;
    d.dateTime2.separator = LAYOUT_SEP_COLON;
    d.dateTime2.font      = LAYOUT_FONT_REGULAR10;
    d.dateTime2.colorR    = 255;
    d.dateTime2.colorG    = 255;
    d.dateTime2.colorB    = 255;
    // Username
    d.username.visible    = 0;
    d.username.x          = 5;
    d.username.y          = 26;
    d.username.font       = LAYOUT_FONT_REGULAR10;
    d.username.colorR     = 255;
    d.username.colorG     = 255;
    d.username.colorB     = 255;
    // Game Title
    d.gameTitle.visible = 0;
    d.gameTitle.x       = 75;
    d.gameTitle.y       = 2;
    d.gameTitle.font    = LAYOUT_FONT_REGULAR10;
    d.gameTitle.colorR  = 255;
    d.gameTitle.colorG  = 255;
    d.gameTitle.colorB  = 255;
    // Prefix
    d.prefix.visible      = 0;
    d.prefix.x            = 160;
    d.prefix.y            = 2;
    d.prefix.font         = LAYOUT_FONT_REGULAR10;
    d.prefix.trailingDash = 0;
    d.prefix.colorR       = 255;
    d.prefix.colorG       = 255;
    d.prefix.colorB       = 255;
    d.prefix.gbaPrefixMode = LAYOUT_PREFIX_GBA_NATIVE;
    d.prefix.ntrPrefixMode = LAYOUT_PREFIX_NTR_NATIVE;
    d.prefix.twlPrefixMode = LAYOUT_PREFIX_TWL_NATIVE;
    // Title ID
    d.TitleID.visible      = 0;
    d.TitleID.x            = 186;
    d.TitleID.y            = 2;
    d.TitleID.font         = LAYOUT_FONT_REGULAR10;
    d.TitleID.trailingDash = 0;
    d.TitleID.colorR       = 255;
    d.TitleID.colorG       = 255;
    d.TitleID.colorB       = 255;
    d.TitleID.showLabelText = 0;
    d.TitleID.labelFont    = LAYOUT_FONT_REGULAR10;
    d.TitleID.labelX       = d.TitleID.x;
    d.TitleID.labelY       = d.TitleID.y;
    d.TitleID.labelColorR  = 255;
    d.TitleID.labelColorG  = 255;
    d.TitleID.labelColorB  = 255;
    // Region
    d.region.visible      = 0;
    d.region.x            = 216;
    d.region.y            = 2;
    d.region.font         = LAYOUT_FONT_REGULAR10;
    d.region.trailingDash = 0;
    d.region.colorR       = 255;
    d.region.colorG       = 255;
    d.region.colorB       = 255;
    // CRC
    d.crc.visible         = 0;
    d.crc.x               = 180;
    d.crc.y               = 18;
    d.crc.font            = LAYOUT_FONT_REGULAR10;
    d.crc.colorR          = 255;
    d.crc.colorG          = 255;
    d.crc.colorB          = 255;
    // Version
    d.version.visible     = 0;
    d.version.x           = 240;
    d.version.y           = 2;
    d.version.font        = LAYOUT_FONT_REGULAR10;
    d.version.colorR      = 255;
    d.version.colorG      = 255;
    d.version.colorB      = 255;
    // Box Art
    d.boxArt.visible      = 1;
    d.boxArt.x            = 75;
    d.boxArt.y            = 18;
    // Icon
    d.icon.visible        = 1;
    d.icon.x              = 24;
    d.icon.y              = 128;
    // ROM Name rows
    d.romNameRow1.visible = 1;
    d.romNameRow1.x       = 70;
    d.romNameRow1.y       = 122;
    d.romNameRow1.font    = LAYOUT_FONT_MEDIUM11;
    d.romNameRow1.colorR  = 0;
    d.romNameRow1.colorG  = 0;
    d.romNameRow1.colorB  = 0;
    d.romNameRow2.visible = 1;
    d.romNameRow2.x       = 70;
    d.romNameRow2.y       = 137;
    d.romNameRow2.font    = LAYOUT_FONT_REGULAR10;
    d.romNameRow2.colorR  = 0;
    d.romNameRow2.colorG  = 0;
    d.romNameRow2.colorB  = 0;
    d.romNameRow3.visible = 1;
    d.romNameRow3.x       = 70;
    d.romNameRow3.y       = 151;
    d.romNameRow3.font    = LAYOUT_FONT_REGULAR10;
    d.romNameRow3.colorR  = 0;
    d.romNameRow3.colorG  = 0;
    d.romNameRow3.colorB  = 0;
    // File Name
    d.fileName.visible    = 1;
    d.fileName.x          = 18;
    d.fileName.y          = 168;
    d.fileName.font       = LAYOUT_FONT_MEDIUM7_5;
    d.fileName.scroll     = 0;
    d.fileName.scrollSpeed = 3;
    d.fileName.colorR     = 0;
    d.fileName.colorG     = 0;
    d.fileName.colorB     = 0;
    return d;
}