#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Convert TTF fonts to NFT2 binary format for the DSpico launcher.

This tool renders glyphs from a TrueType font using FreeType and packs them
into the NFT2 format consumed by the NDS-side nitroFont2 renderer.

Usage:
    python ttf2nft2.py --input font.ttf --output font.nft2 --size 12
    python ttf2nft2.py --input font.ttf --output font.nft2 --size 7.5 --no-cjk
    python ttf2nft2.py --verify font.nft2
"""

from __future__ import annotations

import argparse
import math
import struct
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# FreeType backend
# ---------------------------------------------------------------------------

try:
    import freetype  # type: ignore

    _HAS_FREETYPE = True
except ImportError:
    _HAS_FREETYPE = False

try:
    from PIL import Image, ImageDraw, ImageFont  # type: ignore

    _HAS_PIL = True
except ImportError:
    _HAS_PIL = False


# ---------------------------------------------------------------------------
# GB2312 character set (Level 1 + Level 2 = 6763 characters)
# ---------------------------------------------------------------------------

def _gb2312_chars() -> List[int]:
    """Return Unicode code points for GB2312 Level 1 + Level 2 (rows 16-87).

    GB2312 Level 1 (rows 16-55): 3755 frequently used characters, ordered by pinyin.
    GB2312 Level 2 (rows 56-87): 3008 less common characters, ordered by radical/stroke.
    Total: 6763 characters.
    """
    chars: List[int] = []
    for row in range(16, 88):  # rows 16..87 inclusive (Level 1 + Level 2)
        for col in range(1, 95):  # columns 1..94
            high = row + 0xA0
            low = col + 0xA0
            gb_bytes = bytes([high, low])
            try:
                char = gb_bytes.decode("gb2312")
                cp = ord(char)
                if cp <= 0xFFFF:
                    chars.append(cp)
            except (UnicodeDecodeError, ValueError):
                continue
    return chars


# ---------------------------------------------------------------------------
# Default character set
# ---------------------------------------------------------------------------

def _build_default_charset(include_cjk: bool = True) -> List[int]:
    """Build the default set of Unicode code points to include."""
    chars: set[int] = set()

    # ASCII printable
    for cp in range(0x0020, 0x0080):
        chars.add(cp)

    # Latin Extended subset
    for cp in range(0x00A0, 0x0100):
        chars.add(cp)
    chars.add(0x0152)  # OE ligature
    chars.add(0x0153)  # oe ligature

    # Common punctuation / symbols (selective)
    for cp in range(0x2010, 0x2028):
        chars.add(cp)
    for cp in range(0x2030, 0x2060):
        chars.add(cp)
    for cp in range(0x2190, 0x21FF):
        chars.add(cp)
    chars.add(0x2260)  # not equal
    chars.add(0x2264)  # <=
    chars.add(0x2265)  # >=
    chars.add(0x266A)  # eighth note
    chars.add(0x266B)  # beamed eighth notes
    chars.add(0x266F)  # music sharp

    if include_cjk:
        # Hiragana
        for cp in range(0x3041, 0x3094):
            chars.add(cp)

        # Katakana
        for cp in range(0x30A1, 0x30F7):
            chars.add(cp)

        # CJK punctuation
        for cp in range(0x3000, 0x3016):
            chars.add(cp)

        # Fullwidth forms
        for cp in range(0xFF01, 0xFF5F):
            chars.add(cp)

        # GB2312 Level 1 + Level 2 (6763 characters)
        for cp in _gb2312_chars():
            chars.add(cp)

    return sorted(chars)


# ---------------------------------------------------------------------------
# Glyph rendering
# ---------------------------------------------------------------------------

class RenderedGlyph:
    """Holds the rendered bitmap and metrics for a single glyph."""

    def __init__(
        self,
        width: int,
        height: int,
        bitmap: bytes,
        spacing_left: int,
        spacing_right: int,
        spacing_top: int,
    ) -> None:
        self.width = width
        self.height = height
        self.bitmap = bitmap  # 4bpp packed, row-major
        self.spacing_left = spacing_left
        self.spacing_right = spacing_right
        self.spacing_top = spacing_top


def _clamp_s8(val: int) -> int:
    return max(-128, min(127, val))


def _clamp_u8(val: int) -> int:
    return max(0, min(255, val))


def _unpack_mono_bitmap(buf: bytes, pitch: int, width: int, height: int) -> List[int]:
    """Unpack a 1-bit FreeType mono bitmap to a flat list of 0/255 values."""
    pixels: List[int] = []
    for y in range(height):
        for x in range(width):
            byte_idx = y * pitch + (x >> 3)
            bit_idx = 7 - (x & 7)  # MSB first
            if byte_idx < len(buf) and (buf[byte_idx] >> bit_idx) & 1:
                pixels.append(255)
            else:
                pixels.append(0)
    return pixels


def _render_glyphs_freetype(
    font_path: str,
    size: float,
    codepoints: List[int],
    bitmap_mode: bool = False,
) -> Tuple[int, int, List[Optional[RenderedGlyph]]]:
    """Render glyphs using freetype-py.

    Returns (ascender_px, descender_px, list_of_glyphs).
    A None entry means the codepoint has no glyph in the font.
    """
    face = freetype.Face(font_path)

    # set_pixel_sizes handles fractional sizes better than set_char_size
    # for small pixel sizes on some fonts (especially TTC collections).
    pixel_h = int(round(size))
    if pixel_h < 1:
        pixel_h = 1
    face.set_pixel_sizes(0, pixel_h)

    metrics = face.size
    ascender = int(math.ceil(metrics.ascender / 64.0))
    descender = int(math.ceil(abs(metrics.descender / 64.0)))

    # Choose render target: MONO for bitmap fonts, NORMAL for outline fonts
    load_flags = freetype.FT_LOAD_RENDER
    if bitmap_mode:
        load_flags |= freetype.FT_LOAD_TARGET_MONO
    else:
        load_flags |= freetype.FT_LOAD_TARGET_NORMAL

    glyphs: List[Optional[RenderedGlyph]] = []

    for cp in codepoints:
        glyph_index = face.get_char_index(cp)
        if glyph_index == 0:
            glyphs.append(None)
            continue

        face.load_glyph(glyph_index, load_flags)
        ft_glyph = face.glyph
        ft_bitmap = ft_glyph.bitmap

        glyph_width = ft_bitmap.width
        glyph_height = ft_bitmap.rows
        bearing_x = ft_glyph.bitmap_left
        bearing_y = ft_glyph.bitmap_top
        advance_x = int(round(ft_glyph.advance.x / 64.0))

        # Convert bitmap to 4bpp packed
        row_bytes = (glyph_width + 1) >> 1
        packed = bytearray(row_bytes * glyph_height)

        buf = ft_bitmap.buffer
        pitch = ft_bitmap.pitch

        if bitmap_mode and ft_bitmap.pixel_mode == 1:
            # FT_PIXEL_MODE_MONO: 1-bit per pixel, MSB first
            mono_pixels = _unpack_mono_bitmap(
                bytes(buf), pitch, glyph_width, glyph_height
            )
            for y in range(glyph_height):
                for x in range(glyph_width):
                    val_8bit = mono_pixels[y * glyph_width + x]
                    val_4bit = 15 if val_8bit > 0 else 0
                    byte_idx = y * row_bytes + (x >> 1)
                    if (x & 1) == 0:
                        packed[byte_idx] |= val_4bit & 0x0F
                    else:
                        packed[byte_idx] |= (val_4bit & 0x0F) << 4
        else:
            # 8-bit grayscale
            for y in range(glyph_height):
                for x in range(glyph_width):
                    val_8bit = buf[y * pitch + x] if (y * pitch + x) < len(buf) else 0
                    # Scale 0-255 to 0-15
                    val_4bit = (val_8bit * 15 + 127) // 255
                    byte_idx = y * row_bytes + (x >> 1)
                    if (x & 1) == 0:
                        packed[byte_idx] |= val_4bit & 0x0F
                    else:
                        packed[byte_idx] |= (val_4bit & 0x0F) << 4

        # spacing_top: offset from ascender line to top of glyph bitmap
        spacing_top = ascender - bearing_y
        # spacing_left: bearing_x
        spacing_left = bearing_x
        # spacing_right: advance - bearing_x - width
        spacing_right = advance_x - bearing_x - glyph_width

        glyphs.append(RenderedGlyph(
            width=glyph_width,
            height=glyph_height,
            bitmap=bytes(packed),
            spacing_left=_clamp_s8(spacing_left),
            spacing_right=_clamp_s8(spacing_right),
            spacing_top=_clamp_s8(spacing_top),
        ))

    return ascender, descender, glyphs


def _render_glyphs_pil(
    font_path: str,
    size: float,
    codepoints: List[int],
) -> Tuple[int, int, List[Optional[RenderedGlyph]]]:
    """Fallback renderer using Pillow.

    Returns (ascender_px, descender_px, list_of_glyphs).
    """
    font = ImageFont.truetype(font_path, int(round(size)))

    # Estimate ascender/descender from font metrics
    ascender = font.getmetrics()[0]
    descender = font.getmetrics()[1]

    glyphs: List[Optional[RenderedGlyph]] = []

    for cp in codepoints:
        char = chr(cp)
        bbox = font.getbbox(char)
        if bbox is None:
            glyphs.append(None)
            continue

        x0, y0, x1, y1 = bbox
        glyph_width = x1 - x0
        glyph_height = y1 - y0

        if glyph_width <= 0 or glyph_height <= 0:
            glyphs.append(None)
            continue

        img = Image.new("L", (glyph_width, glyph_height), 0)
        draw = ImageDraw.Draw(img)
        draw.text((-x0, -y0), char, fill=255, font=font)

        pixels = list(img.getdata())

        row_bytes = (glyph_width + 1) >> 1
        packed = bytearray(row_bytes * glyph_height)

        for y in range(glyph_height):
            for x in range(glyph_width):
                val_8bit = pixels[y * glyph_width + x]
                val_4bit = (val_8bit * 15 + 127) // 255
                byte_idx = y * row_bytes + (x >> 1)
                if (x & 1) == 0:
                    packed[byte_idx] |= val_4bit & 0x0F
                else:
                    packed[byte_idx] |= (val_4bit & 0x0F) << 4

        adv = font.getlength(char)
        spacing_left = x0
        spacing_right = int(round(adv)) - x0 - glyph_width
        spacing_top = y0  # PIL bbox y0 is relative to top of em

        glyphs.append(RenderedGlyph(
            width=glyph_width,
            height=glyph_height,
            bitmap=bytes(packed),
            spacing_left=_clamp_s8(spacing_left),
            spacing_right=_clamp_s8(spacing_right),
            spacing_top=_clamp_s8(spacing_top),
        ))

    return _clamp_u8(ascender), _clamp_u8(descender), glyphs


# ---------------------------------------------------------------------------
# NFT2 binary writer
# ---------------------------------------------------------------------------

NFT2_SIGNATURE = 0x3254464E  # "NFT2" in little-endian


def _build_charmap(
    char_to_glyph: Dict[int, int],
) -> bytes:
    """Build the char map as a sequence of range blocks.

    Consecutive characters that all have glyph mappings are merged into
    a single range block for efficiency.
    """
    if not char_to_glyph:
        return struct.pack("<H", 0)  # end sentinel

    sorted_chars = sorted(char_to_glyph.keys())
    ranges: List[Tuple[int, List[int]]] = []

    start = sorted_chars[0]
    glyph_list = [char_to_glyph[start]]

    for i in range(1, len(sorted_chars)):
        cp = sorted_chars[i]
        prev = sorted_chars[i - 1]
        if cp == prev + 1:
            glyph_list.append(char_to_glyph[cp])
        else:
            ranges.append((start, glyph_list))
            start = cp
            glyph_list = [char_to_glyph[cp]]
    ranges.append((start, glyph_list))

    data = bytearray()
    for start_char, glyph_indices in ranges:
        count = len(glyph_indices)
        data += struct.pack("<HH", count, start_char)
        for gi in glyph_indices:
            data += struct.pack("<H", gi)

    # End sentinel
    data += struct.pack("<H", 0)

    return bytes(data)


def build_nft2(
    font_path: str,
    size: float,
    codepoints: List[int],
    bitmap_mode: bool = False,
) -> bytes:
    """Build an NFT2 binary blob from the given font and character set.

    Args:
        font_path: Path to the TTF/OTF font file.
        size: Font size in pixels.
        codepoints: List of Unicode code points to include.
        bitmap_mode: If True, use 1-bit mono rendering for bitmap fonts.

    Returns:
        The complete NFT2 file content as bytes.
    """
    # Render glyphs
    if _HAS_FREETYPE:
        ascender, descender, raw_glyphs = _render_glyphs_freetype(
            font_path, size, codepoints, bitmap_mode=bitmap_mode,
        )
    elif _HAS_PIL:
        print("Warning: freetype-py not available, falling back to Pillow", file=sys.stderr)
        ascender, descender, raw_glyphs = _render_glyphs_pil(
            font_path, size, codepoints,
        )
    else:
        print("Error: neither freetype-py nor Pillow is available", file=sys.stderr)
        sys.exit(1)

    # Build glyph table and char map
    # Glyph 0 = fallback (empty rectangle)
    glyph_infos: List[RenderedGlyph] = []
    fallback = RenderedGlyph(
        width=0,
        height=0,
        bitmap=b"",
        spacing_left=0,
        spacing_right=max(1, int(round(size * 0.5))),
        spacing_top=0,
    )
    glyph_infos.append(fallback)

    char_to_glyph: Dict[int, int] = {}

    for i, cp in enumerate(codepoints):
        rg = raw_glyphs[i]
        if rg is None:
            # Map characters without visible glyphs (e.g. space) to glyph 0 (fallback)
            char_to_glyph[cp] = 0
            continue
        glyph_idx = len(glyph_infos)
        glyph_infos.append(rg)
        char_to_glyph[cp] = glyph_idx

    glyph_count = len(glyph_infos)
    charmap_data = _build_charmap(char_to_glyph)

    # Compute glyph data and offsets
    glyph_data_parts: List[bytes] = []
    glyph_data_offsets: List[int] = []

    current_offset = 0
    for glyph in glyph_infos:
        glyph_data_offsets.append(current_offset)
        glyph_data_parts.append(glyph.bitmap)
        current_offset += len(glyph.bitmap)

    glyph_data_blob = b"".join(glyph_data_parts)

    # Layout (NDS is 32-bit, pointers are u32):
    # [Header: 20 bytes]
    #   u32 signature (4)
    #   u32 glyphInfoOff (4)
    #   u32 charMapOff (4)
    #   u32 glyphDataOff (4)
    #   u8  ascend (1)
    #   u8  descend (1)
    #   u16 glyphCount (2)
    # [GlyphInfo: 8 * glyphCount bytes]
    # [CharMap: variable]
    # [GlyphData: variable]
    header_size = 20
    glyph_info_off = header_size
    glyph_info_size = 8 * glyph_count
    charmap_off = glyph_info_off + glyph_info_size
    charmap_size = len(charmap_data)
    glyph_data_off = charmap_off + charmap_size

    # Build header
    header = struct.pack(
        "<IIIIBBH",
        NFT2_SIGNATURE,
        glyph_info_off,
        charmap_off,
        glyph_data_off,
        _clamp_u8(ascender),
        _clamp_u8(descender),
        min(glyph_count, 0xFFFF),
    )
    assert len(header) == header_size

    # Build glyph info array
    glyph_info_data = bytearray()
    for i, glyph in enumerate(glyph_infos):
        data_offset = glyph_data_offsets[i]
        if data_offset > 0x00FFFFFF:
            print(
                f"Warning: glyph {i} data offset {data_offset:#x} exceeds 24 bits!",
                file=sys.stderr,
            )
            data_offset &= 0x00FFFFFF

        width = _clamp_u8(glyph.width)
        # Pack: (width << 24) | (dataOffset & 0x00FFFFFF) as u32 LE
        first_u32 = (width << 24) | (data_offset & 0x00FFFFFF)
        glyph_info_data += struct.pack(
            "<IbbBb",
            first_u32,
            glyph.spacing_left,
            glyph.spacing_right,
            _clamp_u8(glyph.height),
            glyph.spacing_top,
        )

    assert len(glyph_info_data) == glyph_info_size

    result = header + bytes(glyph_info_data) + charmap_data + glyph_data_blob

    # Print statistics
    mapped_chars = len(char_to_glyph)
    total_requested = len(codepoints)
    skipped = total_requested - mapped_chars
    print(f"Font size: {size}px, ascender: {ascender}px, descender: {descender}px")
    print(f"Glyphs: {glyph_count} (including fallback), mapped chars: {mapped_chars}")
    if skipped > 0:
        print(f"Skipped: {skipped} codepoints (no glyph in font)")
    print(f"Char map ranges: {charmap_data.count(b'') - 1 if False else _count_ranges(charmap_data)}")
    print(f"File size: {len(result)} bytes ({len(result) / 1024:.1f} KB)")

    return result


def _count_ranges(charmap_data: bytes) -> int:
    """Count the number of range blocks in a serialized char map."""
    offset = 0
    count = 0
    while offset < len(charmap_data):
        block_count = struct.unpack_from("<H", charmap_data, offset)[0]
        if block_count == 0:
            break
        count += 1
        offset += 4 + 2 * block_count
    return count


# ---------------------------------------------------------------------------
# NFT2 verify / dump
# ---------------------------------------------------------------------------

def verify_nft2(file_path: str) -> None:
    """Read an NFT2 file and print its stats for verification."""
    data = Path(file_path).read_bytes()
    if len(data) < 20:
        print("Error: file too small for NFT2 header")
        sys.exit(1)

    sig, glyph_info_off, charmap_off, glyph_data_off, ascend, descend, glyph_count = (
        struct.unpack_from("<IIIIBBH", data, 0)
    )

    if sig != NFT2_SIGNATURE:
        print(f"Error: invalid signature {sig:#010x} (expected {NFT2_SIGNATURE:#010x})")
        sys.exit(1)

    print(f"NFT2 file: {file_path}")
    print(f"  File size:       {len(data)} bytes ({len(data) / 1024:.1f} KB)")
    print(f"  Ascender:        {ascend}px")
    print(f"  Descender:       {descend}px")
    print(f"  Glyph count:     {glyph_count}")
    print(f"  GlyphInfo off:   {glyph_info_off:#x}")
    print(f"  CharMap off:     {charmap_off:#x}")
    print(f"  GlyphData off:   {glyph_data_off:#x}")

    # Parse char map to count ranges and mapped characters
    offset = charmap_off
    range_count = 0
    total_mapped = 0
    while offset < len(data):
        block_count = struct.unpack_from("<H", data, offset)[0]
        if block_count == 0:
            break
        start_char = struct.unpack_from("<H", data, offset + 2)[0]
        range_count += 1
        total_mapped += block_count
        offset += 4 + 2 * block_count

    print(f"  Char map ranges: {range_count}")
    print(f"  Mapped chars:    {total_mapped}")

    # Sample glyph info
    if glyph_count > 0:
        print(f"\n  Sample glyph info (first 5):")
        for i in range(min(5, glyph_count)):
            off = glyph_info_off + i * 8
            first_u32, sl, sr, gh, st = struct.unpack_from("<IbbBb", data, off)
            gw = (first_u32 >> 24) & 0xFF
            do = first_u32 & 0x00FFFFFF
            print(
                f"    [{i}] width={gw}, height={gh}, "
                f"dataOff={do:#x}, "
                f"spacingL={sl}, spacingR={sr}, spacingT={st}"
            )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    """Entry point for the TTF to NFT2 converter."""
    parser = argparse.ArgumentParser(
        description="Convert TTF fonts to NFT2 binary format for DSpico launcher",
    )
    parser.add_argument("--input", "-i", type=str, help="Input TTF font file")
    parser.add_argument("--output", "-o", type=str, help="Output NFT2 file")
    parser.add_argument(
        "--size", "-s", type=float, help="Font size in pixels (e.g. 12 or 7.5)",
    )
    parser.add_argument(
        "--chars", type=str,
        help="File containing additional characters (one per line or continuous string)",
    )
    parser.add_argument(
        "--verify", "-v", type=str, nargs="?", const="__from_output__",
        help="Verify an NFT2 file (reads back and prints stats)",
    )
    parser.add_argument(
        "--no-cjk", action="store_true",
        help="Skip CJK characters (for testing with small files)",
    )
    parser.add_argument(
        "--bitmap", action="store_true",
        help="Bitmap font mode: use 1-bit mono rendering (sharp pixels, no antialiasing)",
    )

    args = parser.parse_args()

    # Verify-only mode (--verify <path> without --input)
    if args.verify is not None and args.verify != "__from_output__" and not args.input:
        verify_nft2(args.verify)
        return

    # Convert mode
    if not args.input:
        parser.error("--input is required")
    if not args.output:
        parser.error("--output is required")
    if args.size is None:
        parser.error("--size is required")

    input_path = args.input
    if not Path(input_path).is_file():
        print(f"Error: input file not found: {input_path}", file=sys.stderr)
        sys.exit(1)

    # Build character set
    codepoints = _build_default_charset(include_cjk=not args.no_cjk)

    # Add extra characters from file
    if args.chars:
        chars_path = Path(args.chars)
        if not chars_path.is_file():
            print(f"Error: chars file not found: {args.chars}", file=sys.stderr)
            sys.exit(1)
        text = chars_path.read_text(encoding="utf-8")
        extra: set[int] = set()
        for ch in text:
            cp = ord(ch)
            if cp > 0x20 and cp <= 0xFFFF and cp not in ('\n', '\r'):
                extra.add(cp)
        existing = set(codepoints)
        new_chars = sorted(extra - existing)
        if new_chars:
            print(f"Adding {len(new_chars)} extra characters from {args.chars}")
            codepoints = sorted(set(codepoints) | extra)

    print(f"Input:  {input_path}")
    print(f"Output: {args.output}")
    print(f"Size:   {args.size}px")
    print(f"Character set: {len(codepoints)} codepoints")
    print()

    nft2_data = build_nft2(input_path, args.size, codepoints, bitmap_mode=args.bitmap)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(nft2_data)
    print(f"\nWritten to {args.output}")

    # Auto-verify if --verify flag was also passed
    if args.verify is not None:
        print()
        verify_nft2(args.output)


if __name__ == "__main__":
    main()
