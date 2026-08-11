#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import struct
import sys
from freetype import Face, FT_PIXEL_MODE_GRAY

NFT2_SIGNATURE = 0x3254464E  # "NFT2"

def clamp_s8(v):
    return max(-128, min(127, v))

def clamp_u8(v):
    return max(0, min(255, v))

def convert_bitmap_to_4bpp(bmp):
    """
    Convert an 8bpp FreeType grayscale bitmap into 4bpp packed format.
    Two pixels per byte: low nibble = left pixel, high nibble = right pixel.
    """
    if bmp.pixel_mode != FT_PIXEL_MODE_GRAY:
        raise ValueError("Only FT_PIXEL_MODE_GRAY is supported.")

    width = bmp.width
    height = bmp.rows
    pitch = bmp.pitch

    out = bytearray()
    row_bytes_4bpp = (width + 1) // 2

    for y in range(height):
        row = bmp.buffer[y * pitch:y * pitch + width]
        dst = bytearray(row_bytes_4bpp)
        for x in range(width):
            v4 = row[x] >> 4
            if x & 1:
                dst[x >> 1] |= (v4 << 4)
            else:
                dst[x >> 1] |= v4
        out.extend(dst)

    return bytes(out)


def build_nft2(font_path, pixel_size, output_path):
    face = Face(font_path)
    face.set_pixel_sizes(0, pixel_size)

    # ------------------------------------------------------------
    # Pass 0: Collect all Unicode codepoints from the font's cmap
    # ------------------------------------------------------------
    charcodes = []
    cc, gi = face.get_first_char()
    while gi != 0:
        charcodes.append(cc)
        cc, gi = face.get_next_char(cc, gi)
    charcodes = sorted(set(charcodes))

    # ------------------------------------------------------------
    # Pass 1: Collect metrics for all glyphs (bitmap not stored)
    # ------------------------------------------------------------
    metrics = []
    for codepoint in charcodes:
        face.load_char(chr(codepoint))
        slot = face.glyph
        bmp = slot.bitmap

        metrics.append({
            "codepoint": codepoint,
            "bitmap_top": slot.bitmap_top,
            "height": bmp.rows,
            "width": bmp.width,
            "bitmap_left": slot.bitmap_left,
            "advance": slot.advance.x >> 6,
        })

    # Ascender = maximum bitmap_top among all glyphs
    ascend = max(m["bitmap_top"] for m in metrics)

    # Descender = maximum (height - bitmap_top)
    descend = max((m["height"] - m["bitmap_top"]) for m in metrics)

    # ------------------------------------------------------------
    # Pass 2: Build glyphInfo and glyphData using finalized ascend
    # ------------------------------------------------------------
    glyph_infos = []
    glyph_data = bytearray()

    # Build contiguous Unicode ranges for charMap
    ranges = []
    start = charcodes[0]
    current = [start]
    prev = start
    for c in charcodes[1:]:
        if c == prev + 1:
            current.append(c)
        else:
            ranges.append(current)
            current = [c]
        prev = c
    ranges.append(current)

    glyph_index_counter = 0
    charmap_entries = []

    metric_map = {m["codepoint"]: m for m in metrics}

    for r in ranges:
        start_char = r[0]
        glyph_indices = []

        for codepoint in r:
            m = metric_map[codepoint]

            # Load bitmap again (FreeType slot is volatile)
            face.load_char(chr(codepoint))
            slot = face.glyph
            bmp = slot.bitmap

            # spacingTop = ascend - bitmap_top (original NFT2 behavior)
            spacingTop = clamp_s8(ascend - m["bitmap_top"])
            spacingLeft = clamp_s8(m["bitmap_left"])
            spacingRight = clamp_s8(m["advance"] - spacingLeft - m["width"])

            dataOffset = len(glyph_data)
            glyph_data.extend(convert_bitmap_to_4bpp(bmp))

            glyph_infos.append({
                "dataOffset": dataOffset,
                "glyphWidth": m["width"],
                "spacingLeft": spacingLeft,
                "spacingRight": spacingRight,
                "glyphHeight": m["height"],
                "spacingTop": spacingTop,
            })

            glyph_indices.append(glyph_index_counter)
            glyph_index_counter += 1

        charmap_entries.append((start_char, glyph_indices))

    glyph_count = len(glyph_infos)

    # ------------------------------------------------------------
    # Build NFT2 binary structure
    # ------------------------------------------------------------
    header_size = 4 + 4 + 4 + 4 + 1 + 1 + 2
    glyph_info_offset = header_size
    glyph_info_size = glyph_count * 8

    charmap_offset = glyph_info_offset + glyph_info_size

    # Build charMap table
    charmap_bytes = bytearray()
    for start_char, glyph_list in charmap_entries:
        count = len(glyph_list)
        charmap_bytes.extend(struct.pack("<HH", count, start_char))
        for gi in glyph_list:
            charmap_bytes.extend(struct.pack("<H", gi))
    charmap_bytes.extend(struct.pack("<HH", 0, 0))  # sentinel

    glyphdata_offset = charmap_offset + len(charmap_bytes)

    # Write header
    out = bytearray()
    out.extend(struct.pack(
        "<IIIIBBH",
        NFT2_SIGNATURE,
        glyph_info_offset,
        charmap_offset,
        glyphdata_offset,
        clamp_u8(ascend),
        clamp_u8(descend),
        glyph_count
    ))

    # Write glyphInfo table
    for g in glyph_infos:
        packed_dw = (g["dataOffset"] & 0xFFFFFF) | ((g["glyphWidth"] & 0xFF) << 24)
        out.extend(struct.pack(
            "<IbbBb",
            packed_dw,
            g["spacingLeft"],
            g["spacingRight"],
            g["glyphHeight"] & 0xFF,
            g["spacingTop"],
        ))

    # Append charMap and glyphData
    out.extend(charmap_bytes)
    out.extend(glyph_data)

    with open(output_path, "wb") as f:
        f.write(out)

    # print("NFT2 font written:", output_path)
    # print("Glyph count:", glyph_count)
    # print("Ascend:", ascend, "Descend:", descend)


def main():
    if len(sys.argv) != 4:
        print("Usage: python3 make_nft2.py font.ttf pixel_size output.nft2")
        sys.exit(1)

    font_path = sys.argv[1]
    pixel_size = int(round(float(sys.argv[2])))
    output_path = sys.argv[3]

    build_nft2(font_path, pixel_size, output_path)


if __name__ == "__main__":
    main()
