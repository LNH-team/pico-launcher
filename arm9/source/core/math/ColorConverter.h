#pragma once
#include "Rgb.h"

class Rgb8To5Table
{
    u8 _table[256];

public:
    constexpr Rgb8To5Table()
        : _table()
    {
        for (u32 i = 0; i < 256; i++)
        {
            u32 value5 = (i * 63 + 255) / (255 * 2);
            if (value5 > 31)
                value5 = 31;
            _table[i] = value5;
        }
    }

    constexpr u8 Rgb8ToRgb5(u32 rgb8) const { return _table[rgb8]; }
};

class Rgb8To6Table
{
    u8 _table[256];

public:
    constexpr Rgb8To6Table()
        : _table()
    {
        for (u32 i = 0; i < 256; i++)
            _table[i] = (i * 63 + 128) / 255;
    }

    constexpr u8 Rgb8ToRgb6(u32 rgb8) const { return _table[rgb8]; }
};

class ColorConverter
{
    static constexpr Rgb8To5Table sRgb8To5Table { };
    static constexpr Rgb8To6Table sRgb8To6Table { };
public:
    /// @brief Unpacks a 15 bit color with format xbbb bbgg gggr rrrr.
    /// @param color The packed color.
    /// @return The unpacked color.
    static Rgb<5, 5, 5> FromXBGR555(u16 color)
    {
        return Rgb<5, 5, 5>(color & 0x1F, (color >> 5) & 0x1F, (color >> 10) & 0x1F);
    }

    /// @brief Packs a 15 bit color with format xbbb bbgg gggr rrrr.
    ///        Note that the color components are not clamped or masked.
    /// @param color The color to pack.
    /// @return The packed color.
    static u16 ToXBGR555(const Rgb<5, 5, 5>& color)
    {
        return color.r | (color.g << 5) | (color.b << 10);
    }

    /// @brief Unpacks a 16 bit color with format Gbbb bbgg gggr rrrr.
    ///        With G an additional lsb of green forming a 6 bit value together with ggggg.
    /// @param color The packed color.
    /// @return The unpacked color.
    static Rgb<5, 6, 5> FromGBGR565(u16 color)
    {
        return Rgb<5, 6, 5>(color & 0x1F, ((color >> 4) & 0x3E) | (color >> 15), (color >> 10) & 0x1F);
    }

    /// @brief Packs a 16 bit color with format Gbbb bbgg gggr rrrr.
    ///        With G an additional lsb of green forming a 6 bit value together with ggggg.
    ///        Note that the color components are not clamped or masked.
    /// @param color The color to pack.
    /// @return The packed color.
    static u16 ToGBGR565(const Rgb<5, 6, 5>& color)
    {
        return color.r | ((color.g >> 1) << 5) | (color.b << 10) | (color.g << 15);
    }

    /// @brief Packs a 16 bit color with format Gbbb bbgg gggr rrrr.
    ///        With G an additional lsb of green forming a 6 bit value together with ggggg.
    ///        Note that the color components are not clamped or masked.
    /// @param color The color to pack.
    /// @return The packed color.
    static u16 ToGBGR565(const Rgb<8, 8, 8>& color)
    {
        u32 r = sRgb8To5Table.Rgb8ToRgb5(color.r);
        u32 g = sRgb8To6Table.Rgb8ToRgb6(color.g);
        u32 b = sRgb8To5Table.Rgb8ToRgb5(color.b);

        return r | ((g >> 1) << 5) | (b << 10) | (g << 15);
    }
};
