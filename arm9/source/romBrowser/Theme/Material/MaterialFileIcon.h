#pragma once
#include "../../FileType/FileIcon.h"
#include "core/math/Rgb.h"

class MaterialColorScheme;
class IFontRepository;

class MaterialFileIcon : public FileIcon
{
public:
    MaterialFileIcon(const TCHAR* name, const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);

    void UploadGraphics(vu16* vram) override;
    void Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor) override;

protected:
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;

    virtual const void* GetIconTiles() const = 0;
    virtual Rgb<8, 8, 8> GetIconColor() const = 0;
    virtual Rgb<8, 8, 8> GetTextColor() const = 0;
    virtual int GetTextYOffset() const = 0;

private:
    char16_t _displayName[4];
};
