#!/usr/bin/env python3
import os
import sys
import polib

def extract_chars_from_po(po_path):
    chars = set()
    po = polib.pofile(po_path)

    for entry in po:
        # extract characters from msgid and msgstr 
        for text in (entry.msgid, entry.msgstr):
            if text:
                for ch in text:
                    # control character
                    if ord(ch) >= 0x20:
                        chars.add(ch)
    return chars


def collect_used_chars(po_dir):
    all_chars = set()

    for root, dirs, files in os.walk(po_dir):
        for f in files:
            if f.endswith(".po"):
                path = os.path.join(root, f)
                # print(f"Processing {path}")
                all_chars |= extract_chars_from_po(path)

    return all_chars


def save_chars(chars, out_path):
    sorted_chars = sorted(chars, key=lambda c: ord(c))

    with open(out_path, "w", encoding="utf-8") as f:
        for ch in sorted_chars:
            f.write(ch)

    # print(f"Saved {len(sorted_chars)} characters to {out_path}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: gen_used_chars.py <po_directory> <output_file>")
        sys.exit(1)

    po_directory = sys.argv[1]
    output_file = sys.argv[2]

    chars = collect_used_chars(po_directory)
    save_chars(chars, output_file)
