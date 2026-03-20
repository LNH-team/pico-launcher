# DSpico 开机画面制作指南

## 文件结构

```
开机画面制作/
├── 生成开机画面.py    ← 生成脚本
├── icon.png           ← 图标文件（需要自己放）
├── WenQuanYi Bitmap Song 12px.ttf  ← 点阵字体（需要自己放）
├── 说明.md            ← 本文件
├── 提示词.md          ← AI 生成图标的提示词
└── output/
    ├── splashTop.png  ← 生成的开机画面（用这个替换）
    └── 预览.png       ← RGB 预览图
```

## 快速使用

1. 把图标文件放到本目录，命名为 `icon.png`
2. 把字体文件放到本目录（或脚本会自动查找备选路径）
3. 修改 `生成开机画面.py` 顶部的 CONFIG 区域（文字、颜色、位置）
4. 运行：
   ```bash
   cd /Users/liujiaye/Developer/dspico/launcher-中文增强版/开机画面制作
   /opt/homebrew/bin/python3 生成开机画面.py
   ```
5. 替换并编译：
   ```bash
   cp output/splashTop.png /Users/liujiaye/Developer/dspico/launcher/arm9/gfx/splashTop.png
   cd /Users/liujiaye/Developer/dspico/launcher
   docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-latest sh -c "make clean && make"
   ```

## 可修改的参数

打开 `生成开机画面.py`，修改 CONFIG 区域：

| 参数 | 说明 | 示例 |
|------|------|------|
| `ICON_PATH` | 图标文件路径 | `"icon.png"` |
| `ICON_SIZE` | 图标显示大小 | `(56, 72)` |
| `ICON_Y` | 图标纵坐标 | `20` |
| `BG_COLOR` | 背景颜色 | `(255, 255, 255)` 白色 |
| `TEXTS` | 文字列表 | 见脚本内注释 |
| `MAX_COLORS` | 最大颜色数 | `24`（越少越安全） |

## NDS 瓦片系统限制

NDS 用 8×8 像素的瓦片拼接画面，关键限制：

- **最多 192 个不同瓦片**（超过会乱码）
- **调色板索引 0 = 透明**（脚本已自动处理）
- 纯色区域只占 1 个瓦片，渐变会产生大量瓦片
- 建议：背景用纯色，图案简单，颜色少

## 字体说明

使用文泉驿点阵宋体 12px，渲染尺寸**必须是 15**。

原因：字体 UPM=1500，网格步长=100，1500÷100=15，只有 size=15 时每个像素点精确对齐一个屏幕像素，不会模糊。

## 依赖

```bash
pip install freetype-py Pillow
```
