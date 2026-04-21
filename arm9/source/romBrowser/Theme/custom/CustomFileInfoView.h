#pragma once
#include "core/task/TaskQueue.h"
#include "gui/views/Label2DView.h"
#include "../../views/BannerView.h"

class FileIcon;
class IFontRepository;
class CustomThemeInfo;

class CustomFileInfoView : public BannerView
{
    SHARED_ONLY(CustomFileInfoView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool ellipsis) override
    {
        _firstLine->SetEllipsisStyle(ellipsis ? LabelView::EllipsisStyle::Ellipsis : LabelView::EllipsisStyle::None);
        if (taskQueue)
            _firstLine->SetTextAsync(taskQueue, firstLine);
        else
            _firstLine->SetText(firstLine);
    }

    void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool ellipsis) override
    {
        _firstLine->SetEllipsisStyle(ellipsis ? LabelView::EllipsisStyle::Ellipsis : LabelView::EllipsisStyle::None);
        if (taskQueue)
            _firstLine->SetTextAsync(taskQueue, firstLine, length);
        else
            _firstLine->SetText(firstLine, length);
    }

    void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine, u32 length) override
    {
        if (taskQueue)
            _secondLine->SetTextAsync(taskQueue, secondLine, length);
        else
            _secondLine->SetText(secondLine, length);
    }

    void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine, u32 length) override
    {
        if (taskQueue)
            _thirdLine->SetTextAsync(taskQueue, thirdLine, length);
        else
            _thirdLine->SetText(thirdLine, length);
    }

    void SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle) override
    {
        BannerView::SetFileNameAsync(taskQueue, fileName, useAsTitle);
        if (taskQueue)
            _filenameLabelView->SetTextAsync(taskQueue, fileName);
        else
            _filenameLabelView->SetText(fileName);
    }

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 236, 60);
    }

private:
    SharedPtr<Label2DView> _firstLine;
    SharedPtr<Label2DView> _secondLine;
    SharedPtr<Label2DView> _thirdLine;
    SharedPtr<Label2DView> _filenameLabelView;
    const CustomThemeInfo* _customThemeInfo;

    CustomFileInfoView(const CustomThemeInfo* customThemeInfo, const IFontRepository* fontRepository);
};
