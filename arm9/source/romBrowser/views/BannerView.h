#pragma once
#include "gui/views/ViewContainer.h"
#include "../FileType/FileIcon.h"
#include "services/Layout/LayoutData.h"

class TaskQueueBase;

class BannerView : public ViewContainer
{
public:
    struct LayoutConfig
    {
        bool iconVisible = true;
        int iconX = 24;
        int iconY = 128;

        bool romNameRow1Visible = true;
        int romNameRow1X = 70;
        int romNameRow1Y = 122;
        u8 romNameRow1Font = LAYOUT_FONT_MEDIUM11;
        u8 romNameRow1ColorR = 0;
        u8 romNameRow1ColorG = 0;
        u8 romNameRow1ColorB = 0;

        bool romNameRow2Visible = true;
        int romNameRow2X = 70;
        int romNameRow2Y = 137;
        u8 romNameRow2Font = LAYOUT_FONT_REGULAR10;
        u8 romNameRow2ColorR = 0;
        u8 romNameRow2ColorG = 0;
        u8 romNameRow2ColorB = 0;

        bool romNameRow3Visible = true;
        int romNameRow3X = 70;
        int romNameRow3Y = 151;
        u8 romNameRow3Font = LAYOUT_FONT_REGULAR10;
        u8 romNameRow3ColorR = 0;
        u8 romNameRow3ColorG = 0;
        u8 romNameRow3ColorB = 0;

        bool fileNameVisible = true;
        int fileNameX = 18;
        int fileNameY = 168;
        u8 fileNameFont = LAYOUT_FONT_MEDIUM7_5;
        u8 fileNameColorR = 0;
        u8 fileNameColorG = 0;
        u8 fileNameColorB = 0;
        bool fileNameScrollEnabled = false;
        u8 fileNameScrollSpeed = 3;
    };

    void InitVram(const VramContext& vramContext) override;

    void SetFileName(const TCHAR* fileName, bool useAsTitle)
    {
        SetFileNameAsync(nullptr, fileName, useAsTitle);
    }

    virtual void SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle);

    void SetGameTitle(const char16_t* gameTitle)
    {
        SetGameTitleAsync(nullptr, gameTitle);
    }

    void SetGameTitleAsync(TaskQueueBase* taskQueue, const char16_t* gameTitle);

    void SetIcon(std::unique_ptr<FileIcon> icon)
    {
        _icon = std::move(icon);
    }

    void UploadIconGraphics() const
    {
        if (_icon && _layoutConfig.iconVisible)
        {
            _icon->UploadGraphics(_iconVram);
        }
    }

    void SetLayoutConfig(const LayoutConfig& config)
    {
        _layoutConfig = config;
    }

protected:
    const LayoutConfig& GetLayoutConfig() const { return _layoutConfig; }

    std::unique_ptr<FileIcon> _icon = nullptr;
    vu16* _iconVram;
    u32 _iconVramOffset;
    u32 _lines;
    LayoutConfig _layoutConfig;

    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool ellipsis) = 0;
    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool ellipsis) = 0;
    virtual void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine, u32 length) = 0;
    virtual void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine, u32 length) = 0;
};
