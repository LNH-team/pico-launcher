#pragma once

#define NDS_BANNER_VERSION_1    0x0001
#define NDS_BANNER_VERSION_2    0x0002
#define NDS_BANNER_VERSION_3    0x0003
#define NDS_BANNER_VERSION_103  0x0103

typedef struct
{
    u16 version;
    u16 version1Crc;
    u16 version2Crc;
    u16 version3Crc;
    u16 version103AnimCrc;
    u8 reserved[0x16];
} nds_banner_header_t;

#define NDS_BANNER_ANIM_DURATION_CONTROL_FRAME  0

#define NDS_BANNER_ANIM_CONTROL_LOOP            0
#define NDS_BANNER_ANIM_CONTROL_STOP            1

#define NDS_BANNER_TITLE_LANGUAGE_JAPANESE      0
#define NDS_BANNER_TITLE_LANGUAGE_ENGLISH       1
#define NDS_BANNER_TITLE_LANGUAGE_FRENCH        2
#define NDS_BANNER_TITLE_LANGUAGE_GERMAN        3
#define NDS_BANNER_TITLE_LANGUAGE_ITALIAN       4
#define NDS_BANNER_TITLE_LANGUAGE_SPANISH       5
#define NDS_BANNER_TITLE_LANGUAGE_CHINESE       6
#define NDS_BANNER_TITLE_LANGUAGE_KOREAN        7

typedef struct
{
    u8 duration;
    union
    {
        struct
        {
            u8 gfxIdx : 3;
            u8 plttIdx : 3;
            u8 hFlip : 1;
            u8 vFlip : 1;
        };
        u8 control;
    };
} nds_banner_anim_token_t;

#define NDS_BANNER_ANIM_TOKEN_COUNT     64
#define NDS_BANNER_ICON_SIZE            512

typedef struct
{
    u8 iconGfx[8][NDS_BANNER_ICON_SIZE];    
    u16 iconPltt[8][16];
    nds_banner_anim_token_t animTokens[NDS_BANNER_ANIM_TOKEN_COUNT];
} nds_banner_anim_t;

typedef struct
{
    nds_banner_header_t header;
    u8 iconGfx[NDS_BANNER_ICON_SIZE];
    u16 iconPltt[16];
    char16_t title[16][128];
    nds_banner_anim_t animation;
} nds_banner_t;

static_assert(sizeof(nds_banner_t) == 0x23C0, "Invalid size for nds_banner_t");