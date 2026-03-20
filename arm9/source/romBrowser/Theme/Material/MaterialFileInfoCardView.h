#pragma once
#include "core/task/TaskQueue.h"
#include "core/String.h"
#include "gui/views/Label2DView.h"
#include "../../views/BannerView.h"

class FileIcon;
class MaterialColorScheme;
class IFontRepository;

class MaterialFileInfoCardView : public BannerView
{
public:
    MaterialFileInfoCardView(const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool ellipsis) override;
    void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool ellipsis) override;
    void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine, u32 length) override;
    void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine, u32 length) override;

    void SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 236, 60);
    }

private:
    Label2DView _firstLine;
    Label2DView _secondLine;
    Label2DView _thirdLine;
    Label2DView _filenameLabelView;
    u32 _iconCellVramOffset;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;

    char16_t _baseFileName[256] = { 0 };
    char16_t _fileNameScroller[512] = { 0 };
    bool _fileNameScrollPrepared = false;
    int _fileNameScrollCycleQ8 = 0;
    int _fileNameScrollOffsetQ8 = 0;
    int _fileNameScrollPauseFrames = 0;

    void PrepareFileNameScrollIfNeeded(const BannerView::LayoutConfig& cfg);
    void UpdateFileNameScroll(const BannerView::LayoutConfig& cfg);
    void ApplyLayoutFonts(const BannerView::LayoutConfig& cfg);
};
