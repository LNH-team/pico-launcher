#pragma once
#include "common.h"
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "fat/File.h"
#include "fat/FastFileRef.h"

namespace RomHeaderUtil
{
inline constexpr u32 kHeaderReadSize = 512;

inline bool ReadHeader(const FastFileRef& fastFileRef, u8* headerBuffer, u32 length = kHeaderReadSize)
{
    if (!headerBuffer || length == 0)
    {
        return false;
    }

    File file;
    if (file.Open(fastFileRef, FA_READ) != FR_OK)
    {
        return false;
    }

    return file.ReadExact(headerBuffer, length);
}

inline u32 ComputeCrc32(const void* buffer, u32 length)
{
    static constexpr u32 kCrcPoly = 0xEDB88320u;

    if (!buffer)
    {
        return 0;
    }

    u32 crc = ~0u;
    const u8* ptr = static_cast<const u8*>(buffer);
    while (length--)
    {
        crc ^= *ptr++;
        for (int i = 0; i < 8; i++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? kCrcPoly : 0u);
        }
    }

    return crc;
}

inline char ToUpperAscii(char c)
{
    return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
}

inline void CopyTrimmedAsciiField(char* dst, u32 dstLength, const u8* src, u32 srcLength)
{
    if (!dst || dstLength == 0)
    {
        return;
    }

    u32 trimmedLength = srcLength;
    while (trimmedLength > 0)
    {
        char c = static_cast<char>(src[trimmedLength - 1]);
        if (c != 0 && c != ' ')
        {
            break;
        }
        trimmedLength--;
    }

    u32 copyLength = trimmedLength < (dstLength - 1) ? trimmedLength : (dstLength - 1);
    for (u32 i = 0; i < copyLength; i++)
    {
        dst[i] = static_cast<char>(src[i]);
    }
    dst[copyLength] = 0;
}

inline void CopyTrimmedAsciiField(char16_t* dst, u32 dstLength, const u8* src, u32 srcLength)
{
    char asciiBuffer[32];
    CopyTrimmedAsciiField(asciiBuffer, sizeof(asciiBuffer), src, srcLength);
    StringUtil::Copy(dst, asciiBuffer, dstLength);
}

inline const char* GetRegionCodeText(char regionCode)
{
    switch (ToUpperAscii(regionCode))
    {
        case 'A': return "ASI";
        case 'B': return "BRA";
        case 'C': return "CHN";
        case 'D': return "GER";
        case 'E': return "USA";
        case 'F': return "FRA";
        case 'H': return "HOL";
        case 'I': return "ITA";
        case 'J': return "JPN";
        case 'K': return "KOR";
        case 'P': return "EUR";
        case 'Q': return "DEN";
        case 'S': return "SPA";
        case 'U': return "AUS";
        case 'X': return "EUX";
        case 'Y': return "EUY";
        case 'Z': return "EUZ";
        default:  return nullptr;
    }
}

inline void CopyRegionCodeText(char16_t* dst, u32 dstLength, char regionCode)
{
    const char* regionText = GetRegionCodeText(regionCode);
    if (regionText)
    {
        StringUtil::Copy(dst, regionText, dstLength);
        return;
    }

    char fallback[2] = { ToUpperAscii(regionCode), 0 };
    if (fallback[0] == 0)
    {
        if (dstLength > 0)
        {
            dst[0] = 0;
        }
        return;
    }
    StringUtil::Copy(dst, fallback, dstLength);
}

inline void FormatHexU32(char16_t* dst, u32 dstLength, u32 value)
{
    char buffer[16];
    mini_snprintf(buffer, sizeof(buffer), "%08lX", static_cast<unsigned long>(value));
    StringUtil::Copy(dst, buffer, dstLength);
}

inline void FormatUnsignedU32(char16_t* dst, u32 dstLength, u32 value)
{
    char buffer[16];
    mini_snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(value));
    StringUtil::Copy(dst, buffer, dstLength);
}

inline void AppendTrailingDash(char16_t* text, u32 textLength)
{
    if (!text || textLength < 2)
    {
        return;
    }

    u32 i = 0;
    while (i + 1 < textLength && text[i] != 0)
    {
        i++;
    }

    if (i == 0 || i + 1 >= textLength || text[i - 1] == u'-')
    {
        return;
    }

    text[i++] = u'-';
    text[i] = 0;
}
}
