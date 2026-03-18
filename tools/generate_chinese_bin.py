#!/usr/bin/env python3
"""生成 Chinese.bin 翻译文件，供 DSpico Launcher 的 Localization 系统使用。

格式：LANG v1 二进制文件
- Header: "LANG" (4) + version u8 (1) + entryCount u16 LE (2)
- Entries: keyLen u8 + key bytes + valueLen u8 + value UTF-16 LE
"""

import struct
from pathlib import Path

TRANSLATIONS = {
    "display_settings": "显示设置",
    "layout": "布局",
    "sorting": "排序",
    "theme": "主题",
    "language": "语言",
    "dark_mode": "深色模式",
    "game_details": "游戏详情",
    "total_launches": "启动次数",
    "last_launch": "上次启动",
    "cheats": "金手指",
    "favorites": "收藏夹",
    "cheats_not_found": "未找到该游戏的金手指",
    "cheats_dat_missing": "未找到 usrcheat.dat",
    "selected_cheats": "已选金手指",
    "cheats_no_description_available": "无描述信息",
    "information": "信息",
    "information_user": "用户",
    "information_birthdate": "生日",
    "information_favorite_color": "喜欢的颜色",
    "information_console_language": "主机语言",
    "information_message": "消息",
    "information_console": "主机",
    "information_mode": "模式",
    "information_usrcheat_found": "已找到",
    "information_usrcheat_not_found": "未找到",
    "information_unknown": "未知",
    "information_touch": "触屏",
    "information_usrcheat_filename": "usrcheat.dat",
    "information_color_gray": "灰色",
    "information_color_brown": "棕色",
    "information_color_red": "红色",
    "information_color_pink": "粉色",
    "information_color_orange": "橙色",
    "information_color_yellow": "黄色",
    "information_color_yellow_green": "黄绿色",
    "information_color_green": "绿色",
    "information_color_dark_green": "深绿色",
    "information_color_green_blue": "青色",
    "information_color_light_blue": "浅蓝色",
    "information_color_blue": "蓝色",
    "information_color_dark_blue": "深蓝色",
    "information_color_dark_purple": "深紫色",
    "information_color_purple": "紫色",
    "information_color_purple_red": "紫红色",
    "information_language_english": "英语",
    "information_language_french": "法语",
    "information_language_italian": "意大利语",
    "information_language_german": "德语",
    "information_language_spanish": "西班牙语",
    "information_language_japanese": "日语",
    "information_language_unknown": "未知",
    "bgm": "背景音乐",
    "select_bgm": "选择BGM",
    "bgm_random": "随机",
    "select_theme": "选择主题",
}


def build_bin(translations: dict) -> bytes:
    """构建 LANG v1 二进制翻译文件。"""
    entries = []
    for key, value in translations.items():
        key_bytes = key.encode("ascii")
        value_u16 = value.encode("utf-16-le")
        assert len(key_bytes) <= 31
        assert len(value_u16) // 2 <= 63
        entries.append((key_bytes, value_u16))

    # Header
    data = bytearray()
    data += b"LANG"
    data += struct.pack("<BH", 1, len(entries))

    for key_bytes, value_u16 in entries:
        data += struct.pack("B", len(key_bytes))
        data += key_bytes
        data += struct.pack("B", len(value_u16) // 2)
        data += value_u16

    return bytes(data)


def main():
    bin_data = build_bin(TRANSLATIONS)
    output = Path(__file__).parent.parent / "Chinese.bin"
    output.write_bytes(bin_data)
    print(f"生成 {output} ({len(bin_data)} 字节, {len(TRANSLATIONS)} 条翻译)")


if __name__ == "__main__":
    main()
