# DSpico 中文增强版 v3.1

## 项目概述

基于 [MattiaTheBest115/pico-launcher](https://github.com/MattiaTheBest115/pico-launcher) develop 分支，在保留 Mattia 全部新功能（触屏、主题切换、快速菜单、收藏夹、BGM、睡眠模式等）的基础上，添加中文支持和多项增强功能。

**仓库:** https://github.com/chaye7417/pico-launcher/tree/chinese
**PR:** https://github.com/LNH-team/pico-launcher/pull/37

## 代码分支

| 分支 | 说明 |
|------|------|
| `chinese` | **当前主分支** — 基于 Mattia fork + 中文增强 |
| `develop` | 旧版独立中文版（v2.x，保留备用） |

## 功能清单

### 中文支持（代码改动极小，便于跟进上游）
- 中文字体：WenQuanYi Bitmap 12pt，GB2312 全集 6763 字，由 `tools/ttf2nft2.py` 从 TTF 生成
- 中文游戏标题：NdsInternalFileInfo 优先中文 banner（v2+）
- FatFs 代码页 936：支持中文文件名
- Localization 翻译：Chinese.bin（60 条翻译）
- BGM/主题名中文翻译表

### 深色模式
- AppSettings.darkMode 字段
- Theme::SetDarkMode() 实时重新生成 MaterialColorScheme
- RefreshThemeColors() 刷新调色板和背景
- 设置菜单 ChipView 开关
- MaterialFileInfoCardView 文字颜色适配深色背景

### BGM 背景音乐系统
- 155 首 BCSTM（3DS/DSi/Wii/WiiU/Switch Sports/PSV/NS2）
- BgmAdapter：数据驱动分类，7 个平台自动识别
- RecyclerView 二级菜单，分类折叠/展开
- 支持 Off（关闭）/ Random（随机）/ 指定曲目
- 切换后自动重启生效

### 主题选择
- ThemeAdapter：扫描 `/_pico/themes/` 目录
- Built-in / Custom 分类折叠
- RecyclerView 列表选择，当前主题 · 标记
- 选择后自动重启

### 设置菜单
- ChipView 风格（聚焦高亮明显，替代 Mattia 的 ValueLabel）
- ScrollToFocus 支持滚动
- 触屏拖动滚动 + 点击
- 全面国际化（中英双语）

### 3D CoverFlow
- MaterialRomBrowserViewFactory → CoverFlowRecyclerView
- MaterialCoverFlowFileRecyclerAdapter → CoverView（两处都改才能正常显示）

### 封面

**格式要求：** 128×96 像素，256 色（8bpp），BMP3 无压缩，palette index 0/1 需交换（index 0 被硬件当作透明色）。可视区域为左侧 107×96 像素，右侧 21 像素为黑色 padding。

**SD 卡路径：**
- `/_pico/covers/nds/{gameCode}.bmp` — NDS 封面（按 4 字母 gameCode 命名）
- `/_pico/covers/gba/{gameCode}.bmp` — GBA 封面（按 4 字母 gameCode 命名）
- `/_pico/covers/user/{文件名}.bmp` — 自定义封面（按完整文件名匹配，优先级最高）

**封面来源：**
- NDS：[libretro-thumbnails/Nintendo_-_Nintendo_DS](https://github.com/libretro-thumbnails/Nintendo_-_Nintendo_DS) Named_Boxarts + [GameTDB](https://www.gametdb.com) NDS 数据库做 gameCode 映射
- GBA：[libretro-thumbnails/Nintendo_-_Game_Boy_Advance](https://github.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance) Named_Boxarts + libretro serial DAT 做 gameCode 映射

**转换命令（ImageMagick）：**
```bash
magick input.png -resize 107x96! -background black -alpha remove -alpha off -gravity west -extent 128x96 -colors 256 -type Palette -compress None BMP3:output.bmp
```
转换后还需用脚本交换 palette index 0/1（参见 `convert_nds_covers.py` 中的 `swap_index01`）。

### 其他
- 隐藏系统文件（. _ 开头、隐藏属性）
- 自定义开机画面
- GBA 文件关联（GBARunner3）
- 金手指路径：`/_pico/extras/usrcheat.dat`

## 整合包

| 文件 | 大小 | 内容 |
|------|------|------|
| DSpico-中文增强版-v3.1-完整版.7z | 847 MB | 完整版 |
| DSpico-中文增强版-v3.1-精简版.7z | 32 MB | 不含封面/BGM/主题 |

产物存放在 `dist/` 目录下。

## 工具脚本

| 脚本 | 用途 |
|------|------|
| `tools/ttf2nft2.py` | TTF 字体转 NFT2 格式（NDS 字体） |
| `tools/generate_chinese_bin.py` | 生成 Chinese.bin 翻译文件 |
| `tools/convert_nds_covers.py` | 从 libretro-thumbnails + GameTDB 转换 NDS 封面 |
| `tools/convert_gba_covers.py` | 从 libretro-thumbnails + serial DAT 转换 GBA 封面 |
| `tools/fix_cover_index0.py` | 修复封面 palette index 0 透明问题 |
| `tools/compare_cheats.py` | 对比两个金手指库 |
| `tools/merge_cheats.py` | 合并金手指库 |
| `tools/deploy-cheats.sh` | 部署金手指到多内核路径 |

### 字体编译

当前使用 WenQuanYi Bitmap 12pt 字体，包含 GB2312 全集（6763 汉字）+ ASCII + Latin Extended + 常用符号。

**依赖安装：**
```bash
pip install freetype-py Pillow
```

**生成中文字体（含 CJK）：**
```bash
python tools/ttf2nft2.py --input WenQuanYi-Bitmap.ttf --output arm9/data/WenQuanYi-Bitmap-12.nft2 --size 12
```

**生成西文字体（不含 CJK，用于小字号）：**
```bash
python tools/ttf2nft2.py --input NotoSans-Regular.ttf --output arm9/data/NotoSans-7.nft2 --size 7.5 --no-cjk
```

**验证 NFT2 文件：**
```bash
python tools/ttf2nft2.py --verify arm9/data/WenQuanYi-Bitmap-12.nft2
```

字体文件放在 `arm9/data/` 目录，编译时自动嵌入 ROM。源码中通过 `#include "WenQuanYi-Bitmap-12_nft2.h"` 引用（BlocksDS 的 bin2c 自动生成头文件）。

## 编译与部署

**编译（必须用 v1.15.7）：**
```bash
cd launcher-中文增强版
docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-v1.15.7 make
```

**⚠️ 不要用 `slim-latest` 或 `v1.16.0+`！** 这些版本编译出的二进制在实机上频繁进出文件夹会卡死。经测试 `slim-v1.15.7` 是最后一个稳定的编译工具链。

**部署到 SD 卡：** 编译产物为 `LAUNCHER.nds`，部署时需重命名为 `_picoboot.nds`：
```bash
cp LAUNCHER.nds /Volumes/NDS/_picoboot.nds
```

**部署到测试目录（melonDS）：**
```bash
cp LAUNCHER.nds ~/Developer/dspico/melonds_test/_picoboot.nds
```

**启动链：** 烧录卡固件 → `default.nds`（BOOTLOADER）→ `fat:/_picoboot.nds`（launcher）

**注意：** SD 卡上不需要 `LAUNCHER.nds`，只需要 `_picoboot.nds`。

## 跟进上游更新

Mattia 更新时：
```bash
cd launcher-中文增强版
git fetch mattia develop
git merge mattia/develop
# 解决冲突（通常很少，因为我们的改动极小）
docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-v1.15.7 make
cp LAUNCHER.nds /Volumes/NDS/_picoboot.nds
```

## 已知问题

### 频繁进出文件夹卡死 — ⚠️ 未解决
- **现象：** 反复快速进出文件夹，有概率导致完全冻结
- **原因：** Mattia fork 引入的大量新功能（Layout Editor、Top Screen 元数据标签、收藏夹等）增加了内存压力。NDS 只有 4MB RAM，频繁创建/销毁 View 对象导致堆碎片化。LNH 官方原版不卡
- **排查结论：**
  - BlocksDS v1.16.0+ 工具链会加剧此问题（即使 LNH 原版也卡）
  - v1.15.7 工具链下，LNH 原版不卡，Mattia fork 有概率卡
  - 不是单个 commit 引入的，而是累积内存压力到临界点
  - 尝试过视图对象池复用方案（RecyclerView/TopScreenView），未能解决
- **缓解措施：** 使用 `slim-v1.15.7` 编译，避免极快速连续进出文件夹

## 历史问题解决记录

### 问题 1：触屏 + 主题切换 — ✅ 已通过合并 Mattia fork 解决

### 问题 2：akmenunext 中文显示 — ✅ 已解决
- 下载 akmenunext v2.0.5 pico 版，补全中文语言包
- 默认语言设为简体中文

### 问题 3：三内核共享金手指库 — ✅ 已完成
- DSpico: `/_pico/extras/usrcheat.dat`
- TWiLightMenu: `/_nds/TWiLightMenu/extras/usrcheat.dat`
- AKMenu-Next: `/_nds/akmenunext/cheats/usrcheat.dat`

### 问题 4：扩展金手指库 — ✅ 已完成
- 合并后 7091 条目（54 MB）

### 问题 5：GBA 封面图 — ✅ 已完成
- 2044 张 BMP（libretro-thumbnails + GameTDB serial 映射）
