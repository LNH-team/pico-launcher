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

// Sub-menu indices
#define LAYOUT_SUBMENU_DATETIME1    0
#define LAYOUT_SUBMENU_DATETIME2    1
#define LAYOUT_SUBMENU_ROM_IDCODE   2
#define LAYOUT_SUBMENU_BOXART       3
#define LAYOUT_SUBMENU_ICON         4
#define LAYOUT_SUBMENU_ROMNAME_R1   5
#define LAYOUT_SUBMENU_ROMNAME_R2   6
#define LAYOUT_SUBMENU_ROMNAME_R3   7
#define LAYOUT_SUBMENU_FILENAME     8
#define LAYOUT_SUBMENU_COUNT        9

static const char* const kLayoutSubMenuNames[LAYOUT_SUBMENU_COUNT] = {
    "DateTime1",
    "DateTime2",
    "ROM IdCode",
    "Box Art",
    "Icon",
    "ROM Name R1",
    "ROM Name R2",
    "ROM Name R3",
    "File Name",
};

// Layout data structures
struct LayoutDateTime {
    u8  visible;    
    s16 y;
    s16 x;
    u8  format;     
    u8  separator;   
    u8  font;       
};

struct LayoutElement {
    u8  visible;     
    s16 y;
    s16 x;
    u8  font;       
};

struct LayoutElementNoFont {
    u8  visible;    
    s16 y;
    s16 x;
}; 

struct LayoutFilename {
    u8  visible;
    s16 y;
    s16 x;
    u8  font;
    u8  scroll;
    u8  scrollSpeed;
};

// Full layout data
struct LayoutData {
    LayoutDateTime   dateTime1;     
    LayoutDateTime   dateTime2;     
    LayoutElement    romIdCode;     
    LayoutElementNoFont boxArt;     
    LayoutElementNoFont icon;        
    LayoutElement    romNameRow1;   
    LayoutElement    romNameRow2;    
    LayoutElement    romNameRow3;    
    LayoutFilename   fileName;       
};

// Binary file layout:
//   magic[4]   = "LYOT"
//   version[1] = 1
//   data[57|58] = LayoutData (legacy/new)

// Build a Layout with default values
inline LayoutData LayoutData_Default()
{
    LayoutData d;
    // DateTime1
    d.dateTime1.visible   = 0;
    d.dateTime1.y         = 2;
    d.dateTime1.x         = 5;
    d.dateTime1.format    = LAYOUT_FMT_DD_MM_YYYY;
    d.dateTime1.separator = LAYOUT_SEP_SLASH;
    d.dateTime1.font      = LAYOUT_FONT_REGULAR10;
    // DateTime2
    d.dateTime2.visible   = 0;
    d.dateTime2.y         = 14;
    d.dateTime2.x         = 5;
    d.dateTime2.format    = LAYOUT_FMT_HH_mm_ss;
    d.dateTime2.separator = LAYOUT_SEP_COLON;
    d.dateTime2.font      = LAYOUT_FONT_REGULAR10;
    // ROM IdCode
    d.romIdCode.visible   = 0;
    d.romIdCode.y         = 2;
    d.romIdCode.x         = 200;
    d.romIdCode.font      = LAYOUT_FONT_REGULAR10;
    // Box Art
    d.boxArt.visible      = 1;
    d.boxArt.y            = 18;
    d.boxArt.x            = 75;
    // Icon
    d.icon.visible        = 1;
    d.icon.y              = 128;
    d.icon.x              = 24;
    // ROM Name rows
    d.romNameRow1.visible = 1;
    d.romNameRow1.y       = 122;
    d.romNameRow1.x       = 70;
    d.romNameRow1.font    = LAYOUT_FONT_MEDIUM11;
    d.romNameRow2.visible = 1;
    d.romNameRow2.y       = 137;
    d.romNameRow2.x       = 70;
    d.romNameRow2.font    = LAYOUT_FONT_REGULAR10;
    d.romNameRow3.visible = 1;
    d.romNameRow3.y       = 151;
    d.romNameRow3.x       = 70;
    d.romNameRow3.font    = LAYOUT_FONT_REGULAR10;
    // File Name
    d.fileName.visible    = 1;
    d.fileName.y          = 168;
    d.fileName.x          = 18;
    d.fileName.font       = LAYOUT_FONT_MEDIUM7_5;
    d.fileName.scroll     = 0;
    d.fileName.scrollSpeed = 3;
    return d;
}
