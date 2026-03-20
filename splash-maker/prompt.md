# AI 生成图标提示词

## 用 Nano Banana (Gemini CLI) 生成图标

### 基于参考图编辑
```bash
gemini --yolo "/edit 参考图.png '你的提示词'"
```

### 从零生成
```bash
gemini --yolo "/generate '你的提示词'"
```

## 推荐提示词

### 像素风 DS 图标（推荐）
```
Simple pixel art Nintendo DS handheld console icon, front view, dual screens,
flat colors, 8-bit retro style, white background, centered, no text,
very simple and clean, only the console shape
```

### 树莓派风格图标（当前使用）
```
DSpico logo icon, a pixel art raspberry fruit in simple flat style,
white background, centered, no text, no shadows, clean minimal pixel art
```

### 极简风格
```
Minimalist DS console icon, geometric shapes, flat design,
white background, centered, very simple, no details, no text
```

### 复古游戏风
```
Retro 8-bit pixel art game cartridge icon, NDS style,
simple flat colors, white background, centered, no text
```

## 注意事项

- 生成的图标通常很大（1000+像素），脚本会自动缩放
- 白色背景会被自动去除（变透明）
- 图标越简单，最终瓦片数越少，越不容易乱码
- 如果瓦片数超过 192，减小 ICON_SIZE 或简化图标
