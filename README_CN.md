# Pico Launcher 中文增强版 v3.1

基于 [MattiaTheBest115/pico-launcher](https://github.com/MattiaTheBest115/pico-launcher) develop 分支

[English](README_EN.md)

## 功能

### 中文支持
- 文泉驿点阵字体（GB2312 全集 6763 字）
- 游戏标题优先显示中文
- UI 界面中英双语切换
- 支持中文文件名

### 触屏支持（来自 Mattia fork）
- 触屏滑动浏览游戏列表
- 触屏点击选择游戏、切换设置

### BGM 背景音乐
- 内置 155 首 BGM（3DS / DSi / Wii / Wii U / Switch Sports / PS Vita / NS2）
- 分类折叠菜单，支持随机 / 指定 / 关闭

### 主题系统
- 支持多主题选择（内置 + 第三方）
- 分类折叠菜单

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
- 内置 GBARunner3 加载器
- .gba 文件关联已配置

### 多启动器共存
- TWiLight Menu++ v27.23.0
- AKMenu-Next v2.0.5（已配中文）

## 安装

将整合包解压到 SD 卡根目录覆盖即可。

## 整合包下载

| 文件 | 大小 | 说明 |
|------|------|------|
| 完整版 | 847 MB | 全部内容 |
| 精简版 | 32 MB | 不含封面/BGM/主题 |

## 编译

```bash
docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-v1.15.7 make
```
