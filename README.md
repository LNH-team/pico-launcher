# Pico Launcher 中文增强版 v3.1

基于 [MattiaTheBest115/pico-launcher](https://github.com/MattiaTheBest115/pico-launcher) develop 分支

## 功能特性

- 中文字体（GB2312 全集 6763 字）+ 中英双语切换
- 触屏支持
- 155 首 BGM 背景音乐（7 个平台分类）
- 15 个主题（内置 + 第三方）
- 深色模式
- 3D CoverFlow
- 4658 张 NDS 封面 + 2044 张 GBA 封面
- 7091 条金手指
- GBARunner3 加载 GBA 游戏
- TWiLight Menu++ v27.23.0 / AKMenu-Next v2.0.5

## 安装

解压整合包到 SD 卡根目录覆盖即可。

## 编译

```bash
docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-v1.15.7 make
```

