#pragma once
#include "gui/views/ViewContainer.h"
#include "../FileType/FileIcon.h"
#include "core/StringUtil.h"

class TaskQueueBase;

class BannerView : public ViewContainer
{
public:
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

    /// @brief Shows a name the user set, synchronously. The text is copied at
    ///        once, so a buffer that does not outlive the call is fine.
    void SetTitleText(const TCHAR* title)
    {
        _lines = 1;
        SetFirstLineAsync(nullptr, title, true);
        SetSecondLineAsync(nullptr, u"", 0);
        SetThirdLineAsync(nullptr, u"", 0);
    }

    /// @brief Shows the developer on the second line and the publisher on the
    ///        third, the two rows a DS banner gives the pair, instead of running
    ///        them together on one line. A row with nothing in it is left out,
    ///        and a publisher on its own takes the second line so no blank row is
    ///        left above it.
    void SetCreditsText(const char* developer, const char* publisher)
    {
        // the lines take wide text only; copied at once, since the caller's
        // buffer may be a local
        char16_t wideDeveloper[68] = {};
        char16_t widePublisher[68] = {};
        if (developer && developer[0] != 0)
            StringUtil::Copy(wideDeveloper, developer, sizeof(wideDeveloper) / sizeof(wideDeveloper[0]));
        if (publisher && publisher[0] != 0)
            StringUtil::Copy(widePublisher, publisher, sizeof(widePublisher) / sizeof(widePublisher[0]));

        if (wideDeveloper[0] != 0)
        {
            _lines = widePublisher[0] != 0 ? 3 : 2;
            SetSecondLineAsync(nullptr, wideDeveloper);
        }
        else if (widePublisher[0] != 0)
        {
            _lines = 2;
            SetSecondLineAsync(nullptr, widePublisher);
        }
        else
        {
            _lines = 1;
            SetSecondLineAsync(nullptr, u"", 0);
        }
        if (_lines >= 3)
            SetThirdLineAsync(nullptr, widePublisher);
        else
            SetThirdLineAsync(nullptr, u"", 0);
    }

    void SetIcon(std::unique_ptr<FileIcon> icon)
    {
        _icon = std::move(icon);
        if (_icon)
        {
            _icon->SetVramAddress(_iconVram, _iconVramOffset);
        }
    }

    void UploadIconGraphics() const
    {
        if (_icon)
        {
            _icon->UploadGraphics();
        }
    }

protected:
    std::unique_ptr<FileIcon> _icon = nullptr;
    vu16* _iconVram = nullptr;
    u32 _iconVramOffset = 0;
    u32 _lines = 3;

    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool ellipsis) = 0;
    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, bool ellipsis) = 0;
    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool ellipsis) = 0;
    virtual void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine) = 0;
    virtual void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine, u32 length) = 0;
    virtual void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine) = 0;
    virtual void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine, u32 length) = 0;
};
