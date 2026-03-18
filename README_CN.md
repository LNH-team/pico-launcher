# Pico Launcher 中文增强版 v3.0

基于 [MattiaTheBest115/pico-launcher](https://github.com/MattiaTheBest115/pico-launcher) develop 分支

[English](README_EN.md)

## 功能

### 中文支持
- 文泉驿点阵字体，完美显示中文
- 游戏标题优先显示中文
- UI 界面中英双语切换
- 支持中文文件名

### 触屏支持（来自 Mattia fork）
- 触屏滑动浏览游戏列表
- 触屏点击选择游戏、切换设置

### BGM 背景音乐
- 内置 155 首 BGM（3DS / DSi / Wii / Wii U / Switch Sports / PS Vita / NS2）
- 分类折叠菜单，支持随机 / 指定 / 关闭
- BGM 名中文翻译

### 主题系统
- 支持多主题选择（内置 + 第三方）
- 分类折叠菜单
- 主题名中文翻译

### 深色模式
- 设置中一键切换，即时生效

### 3D CoverFlow
- Material 主题下启用 3D 封面旋转效果

### 游戏封面
- 4658 张 NDS 封面（GameTDB）
- 2044 张 GBA 封面（libretro-thumbnails）

### 金手指
- 7091 条目金手指数据库
- 按 Y 键打开金手指面板

### GBA 支持
- 内置 GBARunner2 加载器
- .gba 文件关联已配置

### 多启动器共存
- TWiLight Menu++ v27.23.0
- AKMenu-Next v2.0.5（已配中文）

## 安装

将整合包解压到 SD 卡根目录覆盖即可。

## 整合包下载

| 文件 | 大小 | 说明 |
|------|------|------|
| 完整版 | 811 MB | 全部内容 |
| 精简版 | 33 MB | 不含封面/BGM/主题 |
| NDS 封面 | 36 MB | 4658 张 |
| GBA 封面 | 17 MB | 2044 张 |
| BGM | 597 MB | 155 首 |
| 第三方主题 | 128 MB | 13 个 |

## 设置菜单

按 SELECT 打开，方向键或触屏操作：

| 选项 | 说明 |
|------|------|
| Layout | 显示模式（网格/列表/CoverFlow） |
| Sorting | 排序方式 |
| Theme | 按 A 打开主题列表 |
| Language | 按 A 切换中文/英文 |
| Dark Mode | 按 A 切换深色模式 |
| BGM | 按 A 打开 BGM 列表 |

## 编译

```bash
docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-latest make
```

## 致谢

- [LNH-team](https://github.com/LNH-team/pico-launcher) — pico-launcher 原版
- [MattiaTheBest115](https://github.com/MattiaTheBest115/pico-launcher) — 触屏/主题切换/BGM 等功能
- [DS-Homebrew](https://github.com/DS-Homebrew) — TWiLight Menu++ / nds-bootstrap
- [coderkei](https://github.com/coderkei/akmenu-next) — AKMenu-Next
- 群友 — 金手指库、封面图、第三方主题、BGM 音乐
