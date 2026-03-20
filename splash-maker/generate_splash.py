#!/usr/bin/env python3
"""
DSpico Launcher 开机画面生成工具

用法：
    /opt/homebrew/bin/python3 生成开机画面.py

修改说明：
    1. 修改下方 CONFIG 区域的文字内容
    2. 替换 icon.svg 为你的图标文件
    3. 运行脚本，生成的文件在 output/ 目录
    4. 将 splashTop.png 复制到 launcher/arm9/gfx/ 目录替换
    5. 重新编译 launcher

依赖：
    pip install freetype-py Pillow cairosvg

NDS 瓦片系统限制：
    - 屏幕分辨率：256×192 像素
    - 瓦片大小：8×8 像素
    - 最大不同瓦片数：192 个
    - 调色板索引 0 会显示为透明，不能是白色
    - 颜色越少、背景越统一，瓦片数越少
"""

import os
import sys
import io

# ============================================================
# CONFIG - 修改这里的内容
# ============================================================

# SVG 图标文件路径
ICON_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon.svg")

# 图标大小（宽, 高）
ICON_SIZE = (48, 60)

# 图标位置 Y 坐标（从顶部算起）
ICON_Y = 22

# 背景颜色 (R, G, B)
BG_COLOR = (255, 255, 255)

# SVG 里使用的颜色（用于消除抗锯齿渐变）
# 渲染后每个像素会被吸附到最近的颜色，确保点对点
# 修改图标后需要更新这里的颜色列表
SVG_COLORS = [
    (3, 1, 1),        # #030101 黑色轮廓
    (33, 104, 47),     # #21682f 深绿
    (111, 182, 80),    # #6fb650 浅绿
    (177, 15, 70),     # #b10f46 深粉
    (234, 80, 125),    # #ea507d 浅粉
    (255, 255, 255),   # 白色背景
]

# 文字内容和样式
# 字体：文泉驿点阵宋体 12px，size=15（像素对齐，不要改）
TEXTS = [
    {
        "text": "Pico Launcher",
        "color": (30, 30, 30),
        "y": 95,
    },
    {
        "text": "中文增强版 v3.1",
        "color": (60, 100, 180),
        "y": 115,
    },
    {
        "text": "茶叶 · QQ群 1077276033",
        "color": (150, 150, 150),
        "y": 168,
    },
]

# 字体路径（文泉驿点阵宋体 12px）
FONT_PATHS = [
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "WenQuanYi Bitmap Song 12px.ttf"),
    "/Users/liujiaye/Developer/汉化工作/游戏王2009/WenQuanYi Bitmap Song 12px.ttf",
]

# size=15 时像素完美对齐（UPM=1500, 网格步长=100, 1500/100=15）
FONT_SIZE = 15  # 不要改！

# ============================================================
# 以下是生成逻辑，一般不需要修改
# ============================================================

def find_font():
    for p in FONT_PATHS:
        if os.path.exists(p):
            return p
    print("错误：找不到文泉驿点阵字体！")
    print("请将 'WenQuanYi Bitmap Song 12px.ttf' 放到本目录下")
    sys.exit(1)


def nearest_color(r, g, b):
    """找到 SVG_COLORS 中最接近的颜色"""
    best = SVG_COLORS[0]
    min_d = 999999
    for c in SVG_COLORS:
        d = (r - c[0])**2 + (g - c[1])**2 + (b - c[2])**2
        if d < min_d:
            min_d = d
            best = c
    return best


def render_icon(svg_path, width, height):
    """渲染 SVG 图标，消除抗锯齿，返回 RGBA Image"""
    import cairosvg
    from PIL import Image

    svg_data = open(svg_path, 'rb').read()
    png_data = cairosvg.svg2png(bytestring=svg_data, output_width=width, output_height=height)
    icon = Image.open(io.BytesIO(png_data)).convert('RGBA')

    # 吸附每个像素到 SVG 实际颜色，消除抗锯齿渐变
    px = icon.load()
    for y in range(icon.height):
        for x in range(icon.width):
            r, g, b, a = px[x, y]
            if a < 128:
                px[x, y] = (255, 255, 255, 0)
            else:
                nr, ng, nb = nearest_color(r, g, b)
                if nr == 255 and ng == 255 and nb == 255:
                    px[x, y] = (255, 255, 255, 0)
                else:
                    px[x, y] = (nr, ng, nb, 255)

    return icon


def draw_text(canvas, text, color, y, face):
    """用 freetype MONO 模式绘制点阵文字（点对点，无抗锯齿）"""
    import freetype

    chars = []
    total_w = 0
    max_top = 0
    for ch in text:
        face.load_char(ch, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
        g = face.glyph
        bm = g.bitmap
        chars.append({
            'w': bm.width, 'h': bm.rows,
            'left': g.bitmap_left, 'top': g.bitmap_top,
            'adv': g.advance.x >> 6,
            'buf': bytes(bm.buffer), 'pitch': bm.pitch
        })
        total_w += g.advance.x >> 6
        if g.bitmap_top > max_top:
            max_top = g.bitmap_top

    sx = (256 - total_w) // 2
    for c in chars:
        for row in range(c['h']):
            for col in range(c['w']):
                bi = row * c['pitch'] + (col >> 3)
                bit = 7 - (col & 7)
                if bi < len(c['buf']) and (c['buf'][bi] >> bit) & 1:
                    xx = sx + c['left'] + col
                    yy = y + max_top - c['top'] + row
                    if 0 <= xx < 256 and 0 <= yy < 192:
                        canvas.putpixel((xx, yy), color)
        sx += c['adv']


def convert_to_nds(canvas):
    """转换为 NDS 兼容的索引色 PNG"""
    from PIL import Image

    # 统一白色背景
    cpx = canvas.load()
    for y in range(192):
        for x in range(256):
            r, g, b = cpx[x, y]
            if r > 220 and g > 220 and b > 220:
                cpx[x, y] = (255, 255, 255)

    # 收集所有颜色，手动构建调色板
    # 索引 0 = 黑色（避免透明问题）
    all_colors = [(0, 0, 0)]
    seen = {(0, 0, 0)}
    for y in range(192):
        for x in range(256):
            c = cpx[x, y]
            if c not in seen:
                seen.add(c)
                all_colors.append(c)

    color_to_idx = {c: i for i, c in enumerate(all_colors)}
    white_idx = color_to_idx.get((255, 255, 255), 1)

    # 创建索引色图片
    canvas_p = Image.new('P', (256, 192))
    flat_pal = []
    for c in all_colors:
        flat_pal.extend(c)
    flat_pal.extend([0] * (768 - len(flat_pal)))
    canvas_p.putpalette(flat_pal)

    ppx = canvas_p.load()
    for y in range(192):
        for x in range(256):
            c = cpx[x, y]
            ppx[x, y] = color_to_idx.get(c, white_idx)

    return canvas_p, len(all_colors)


def count_tiles(canvas_p):
    """统计 8x8 瓦片数"""
    ppx = canvas_p.load()
    tiles = set()
    for ty in range(24):
        for tx in range(32):
            tile = tuple(ppx[tx*8+p, ty*8+q] for q in range(8) for p in range(8))
            tiles.add(tile)
    return len(tiles)


def main():
    try:
        import freetype
    except ImportError:
        print("错误：需要 freetype-py")
        print("安装：pip install freetype-py")
        sys.exit(1)

    try:
        import cairosvg
    except ImportError:
        print("错误：需要 cairosvg")
        print("安装：pip install cairosvg")
        sys.exit(1)

    from PIL import Image

    font_path = find_font()
    face = freetype.Face(font_path)
    face.set_pixel_sizes(0, FONT_SIZE)
    print(f"字体：{font_path}")

    # 创建画布
    canvas = Image.new('RGB', (256, 192), BG_COLOR)

    # 渲染图标
    if os.path.exists(ICON_PATH):
        icon = render_icon(ICON_PATH, ICON_SIZE[0], ICON_SIZE[1])
        canvas.paste(icon, ((256 - ICON_SIZE[0]) // 2, ICON_Y), icon)
        print(f"图标：{ICON_PATH} ({ICON_SIZE[0]}x{ICON_SIZE[1]}) 颜色已吸附")
    else:
        print(f"警告：图标文件不存在 {ICON_PATH}")

    # 绘制文字
    for item in TEXTS:
        draw_text(canvas, item["text"], item["color"], item["y"], face)
        print(f"文字：\"{item['text']}\"")

    # 转 NDS 格式
    canvas_p, color_count = convert_to_nds(canvas)
    tile_count = count_tiles(canvas_p)

    print(f"\n=== 结果 ===")
    print(f"颜色数：{color_count}")
    print(f"瓦片数：{tile_count} / 192")

    if tile_count > 192:
        print("⚠️ 瓦片超限！需要简化图标或减少颜色")
    else:
        print("✅ 合规")

    # 保存
    output_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "output")
    os.makedirs(output_dir, exist_ok=True)

    canvas_p.save(os.path.join(output_dir, "splashTop.png"), optimize=False)
    canvas.save(os.path.join(output_dir, "预览.png"))

    print(f"\n已保存到 {output_dir}/")
    print(f"\n下一步：")
    print(f"  cp '{output_dir}/splashTop.png' /Users/liujiaye/Developer/dspico/launcher/arm9/gfx/splashTop.png")
    print(f"  cd /Users/liujiaye/Developer/dspico/launcher")
    print(f"  docker run --rm -v \"$(pwd):/work\" -w /work skylyrac/blocksds:slim-latest sh -c \"make clean && make\"")


if __name__ == "__main__":
    main()
