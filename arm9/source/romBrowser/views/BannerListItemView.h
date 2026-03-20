#pragma once
#include "BannerView.h"
#include "gui/views/LabelView.h"
#include "../FileType/FileIcon.h"

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

    BannerListItemView(std::unique_ptr<LabelView> firstLine,
        std::unique_ptr<LabelView> secondLine, std::unique_ptr<LabelView> thirdLine);

    void Update() override;

    virtual void SetGraphics(const VramToken& vramToken) { }

    void SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool ellipsis) override
    {
        if (taskQueue)
            _firstLine->SetTextAsync(taskQueue, firstLine);
        else
            _firstLine->SetText(firstLine);
    }

    void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool ellipsis) override
    {
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

protected:
    std::unique_ptr<LabelView> _firstLine;
    std::unique_ptr<LabelView> _secondLine;
    std::unique_ptr<LabelView> _thirdLine;
};
