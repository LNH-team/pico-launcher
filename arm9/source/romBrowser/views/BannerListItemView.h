#pragma once
#include "BannerView.h"
#include "gui/views/LabelView.h"
#include "../FileType/FileIcon.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "RomBrowserItemInputHandler.h"

class BannerListItemView : public BannerView
{
public:
    class VramToken
    {
        u32 _vramOffset;
    public:
        VramToken()
            : _vramOffset(0) { }

        explicit VramToken(u32 offset)
            : _vramOffset(offset) { }

        constexpr u32 GetVramOffset() const { return _vramOffset; }
    };

    void Update() override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    virtual void SetGraphics(const VramToken& vramToken) { }

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

    RomBrowserItemViewModel& GetViewModel() const
    {
        return *_viewModel;
    }

protected:
    std::unique_ptr<RomBrowserItemViewModel> _viewModel;
    SharedPtr<LabelView> _firstLine;
    SharedPtr<LabelView> _secondLine;
    SharedPtr<LabelView> _thirdLine;
    RomBrowserItemInputHandler _inputHandler;

    BannerListItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel, SharedPtr<LabelView> firstLine,
        SharedPtr<LabelView> secondLine, SharedPtr<LabelView> thirdLine);
};
