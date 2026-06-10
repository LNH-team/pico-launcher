#pragma once
#include "FileIcon.h"
#include "BmpFileIconData.h"
#include "core/SharedPtr.h"

/// @brief Icon of a file loaded from an external 32x32 4 bpp 16-color BMP file.
class BmpFileIcon : public FileIcon
{
public:
    explicit BmpFileIcon(SharedPtr<BmpFileIconData> iconData)
        : _iconData(std::move(iconData)) { }

    void UploadGraphics() override;

    void Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor) override;

private:
    SharedPtr<BmpFileIconData> _iconData;
};
