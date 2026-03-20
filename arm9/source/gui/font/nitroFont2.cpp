#include "common.h"
#include "nitroFont2.h"

bool nft2_unpack(nft2_header_t* font)
{
    if (font->signature != NFT2_SIGNATURE)
        return false;

    font->glyphInfoPtr = (const nft2_glyph_t*)((u32)font + (u32)font->glyphInfoPtr);
    font->charMapPtr = (const nft2_char_map_entry_t*)((u32)font + (u32)font->charMapPtr);
    font->glyphDataPtr = (const u8*)((u32)font + (u32)font->glyphDataPtr);

    return true;
}

int nft2_findGlyphIdxForCharacter(const nft2_header_t* font, u16 character)
{
    const nft2_char_map_entry_t* charMapEntry = font->charMapPtr;
    while (charMapEntry->count > 0)
    {
        if (charMapEntry->startChar <= character && character < charMapEntry->startChar + charMapEntry->count)
            return charMapEntry->glyphs[character - charMapEntry->startChar];

        charMapEntry = (const nft2_char_map_entry_t*)((u32)charMapEntry + 4 + 2 * charMapEntry->count);
    }

    return 0;
}

static inline void renderGlyph(const nft2_header_t* font, const nft2_glyph_t* glyph,
    int xPos, int yPos, int width, int height, u8* dst, u32 stride, bool a5i3)
{
    int yOffset = glyph->spacingTop;
    u32 xStart = xPos < 0 ? -xPos : 0;
    u32 yStart = yPos + yOffset < 0 ? -(yPos + yOffset) : 0;

    int xEnd = glyph->glyphWidth;
    if (xPos + xEnd > width)
    {
        // by returning we only render complete glyphs
        return;
        // old code for rendering partial glyphs
        // xEnd = width - xPos;
    }

    int yEnd = glyph->glyphHeight;
    if (yPos + yOffset + yEnd > height)
        yEnd = height - (yPos + yOffset); // allow partial glyphs in the vertical direction

    const u8* glyphData = &font->glyphDataPtr[glyph->dataOffset];
    glyphData += yStart * ((glyph->glyphWidth + 1) >> 1);
    for (int y = yStart; y < yEnd; y++)
    {
        for (int x = xStart; x < xEnd; x++)
        {
            u32 data = glyphData[x >> 1];
            if ((x & 1) == 0)
                data &= 0xF;
            else
                data >>= 4;

            if (data == 0)
                continue;

            u32 finalX = x + xPos;
            u32 finalY = y + yPos + yOffset;

            if (a5i3)
            {
                dst[finalY * stride + finalX] = (data << 4) | (data ? (1 << 3) : 0);
            }
            else
            {
                u32 tileX = finalX >> 3;
                u32 tileY = finalY >> 3;

                u32 tileIdx = (tileY >> 1) * (stride >> 2) + (tileX >> 2) * 8 + (tileY & 1) * 4 + (tileX & 3);

                u32 offset = tileIdx * 64 + ((finalY & 7) << 3) + (finalX & 7);
                u32 value = dst[offset >> 1];
                if (offset & 1)
                {
                    u32 prevData = value >> 4;
                    if (prevData < data)
                        dst[offset >> 1] = (value & 0xF) | (data << 4);
                }
                else
                {
                    u32 prevData = value & 0xF;
                    if (prevData < data)
                        dst[offset >> 1] = (value & 0xF0) | data;
                }
            }
        }
        glyphData += (glyph->glyphWidth + 1) >> 1;
    }
}

static ITCM_CODE void renderGlyphTiled(const nft2_header_t* font, const nft2_glyph_t* glyph,
    int xPos, int yPos, int width, int height, u8* dst, u32 stride)
{
    renderGlyph(font, glyph, xPos, yPos, width, height, dst, stride, false);
}

static ITCM_CODE void renderGlyphA5I3(const nft2_header_t* font, const nft2_glyph_t* glyph,
    int xPos, int yPos, int width, int height, u8* dst, u32 stride)
{
    renderGlyph(font, glyph, xPos, yPos, width, height, dst, stride, true);
}

ITCM_CODE void nft2_renderString(const nft2_header_t* font, const char16_t* string, u8* dst, u32 stride,
    nft2_string_render_params_t* renderParams)
{
    int xPos = renderParams->x;
    int yPos = renderParams->y;
    bool a5i3 = renderParams->a5i3;
    u32 textWidth = 0;
    while (true)
    {
        u16 c = *string++;
        if (c == 0)
            break;
        if (c == '\n')
        {
            xPos = renderParams->x;
            yPos += font->ascend + font->descend - 1;
            if (yPos >= (int)renderParams->height)
                break;
            continue;
        }

        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        xPos += glyph->spacingLeft;
        if (a5i3)
        {
            renderGlyphA5I3(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        else
        {
            renderGlyphTiled(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        xPos += glyph->glyphWidth;
        if (xPos > (int)textWidth)
            textWidth = xPos;
        xPos += glyph->spacingRight;
    }
    renderParams->textWidth = textWidth;
}

ITCM_CODE void nft2_measureString(const nft2_header_t* font, const char16_t* string, u32& width, u32& height)
{
    int xPos = 0;
    int yPos = 0;
    u32 textWidth = 0;
    while (true)
    {
        u16 c = *string++;
        if (c == 0)
            break;
        if (c == '\n')
        {
            xPos = 0;
            yPos += font->ascend + font->descend - 1;
            continue;
        }

        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        xPos += glyph->spacingLeft;
        xPos += glyph->glyphWidth;
        if (xPos > (int)textWidth)
            textWidth = xPos;
        xPos += glyph->spacingRight;
    }
    width = textWidth;
    height = yPos + font->ascend + font->descend;
}

static ITCM_CODE const char16_t* findFirstCharacterThatDoesNotFit(const nft2_header_t* font, const char16_t* string, u32 width)
{
    int xPos = 0;
    while (true)
    {
        u16 c = *string++;
        if (c == 0 || c == '\n')
            return string - 1;

        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        xPos += glyph->spacingLeft + glyph->glyphWidth + glyph->spacingRight;
        if (xPos > (int)width)
            return string - 1;
    }
}

static ITCM_CODE const char16_t* findLastCharacterThatFitsBackwards(const nft2_header_t* font, const char16_t* stringStart, const char16_t* stringEnd, u32 width)
{
    const char16_t* string = stringEnd;
    int xPos = (int)width;
    while (string >= stringStart)
    {
        u16 c = *--string;
        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        if (string != stringEnd - 1)
            xPos -= glyph->spacingRight;
        xPos -= glyph->glyphWidth;
        xPos -= glyph->spacingLeft;
        if (xPos < 0)
            return string + 1;
    }
    return stringStart;
}

static ITCM_CODE int measureEllipsisWidth(const nft2_header_t* font, const char16_t* ellipsisString)
{
    int ellipsisWidth = 0;
    const char16_t* stringPtr = ellipsisString;
    while (true)
    {
        u16 c = *stringPtr++;
        if (c == 0)
            break;

        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        ellipsisWidth += glyph->spacingLeft + glyph->glyphWidth + glyph->spacingRight;
    }
    return ellipsisWidth;
}

ITCM_CODE void nft2_renderStringEllipsis(const nft2_header_t* font, const char16_t* string, u8* dst,
    u32 stride, nft2_string_render_params_t* renderParams, const char16_t* ellipsisString)
{
    u32 stringWidth, stringHeight;
    nft2_measureString(font, string, stringWidth, stringHeight);
    if (stringWidth <= renderParams->width)
    {
        // no ellipsis needed
        nft2_renderString(font, string, dst, stride, renderParams);
        return;
    }
    u32 splitPoint = (renderParams->width - renderParams->x) * 3 / 4;
    int ellipsisWidth = measureEllipsisWidth(font, ellipsisString);
    u32 splitLeftEnd = splitPoint - (ellipsisWidth >> 1);
    u32 splitRightStart = splitPoint + ((ellipsisWidth + 1) >> 1);
    const char16_t* endOfFirstPart = findFirstCharacterThatDoesNotFit(font, string, splitLeftEnd);
    u32 stringLength = 0;
    while (string[stringLength] != 0)
        stringLength++;
    const char16_t* startOfSecondPart = findLastCharacterThatFitsBackwards(
        font, endOfFirstPart, string + stringLength, renderParams->width - renderParams->x - splitRightStart);

    int xPos = renderParams->x;
    int yPos = renderParams->y;
    bool a5i3 = renderParams->a5i3;
    u32 textWidth = 0;
    const char16_t* stringPtr = string;
    while (stringPtr < endOfFirstPart)
    {
        u16 c = *stringPtr++;
        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        xPos += glyph->spacingLeft;
        if (a5i3)
        {
            renderGlyphA5I3(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        else
        {
            renderGlyphTiled(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        xPos += glyph->glyphWidth;
        if (xPos > (int)textWidth)
            textWidth = xPos;
        xPos += glyph->spacingRight;
    }
    stringPtr = ellipsisString;
    while (true)
    {
        u16 c = *stringPtr++;
        if (c == 0)
            break;
        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        xPos += glyph->spacingLeft;
        if (a5i3)
        {
            renderGlyphA5I3(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        else
        {
            renderGlyphTiled(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        xPos += glyph->glyphWidth;
        if (xPos > (int)textWidth)
            textWidth = xPos;
        xPos += glyph->spacingRight;
    }
    stringPtr = startOfSecondPart;
    while (true)
    {
        u16 c = *stringPtr++;
        if (c == 0)
            break;
        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        xPos += glyph->spacingLeft;
        if (a5i3)
        {
            renderGlyphA5I3(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        else
        {
            renderGlyphTiled(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        }
        xPos += glyph->glyphWidth;
        if (xPos > (int)textWidth)
            textWidth = xPos;
        xPos += glyph->spacingRight;
    }
    renderParams->textWidth = textWidth;
}