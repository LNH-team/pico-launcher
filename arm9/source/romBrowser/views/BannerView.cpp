#include "common.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "BannerView.h"

void BannerView::InitVram(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _iconVramOffset = objVramManager->Alloc(FILE_ICON_VRAM_SIZE);
        _iconVram = objVramManager->GetVramAddress(_iconVramOffset);
    }

    ViewContainer::InitVram(vramContext);
}

void BannerView::SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle)
{
    if (useAsTitle)
    {
        _lines = 1;
        SetFirstLineAsync(taskQueue, fileName, true);
        SetSecondLineAsync(taskQueue, u"", 0);
        SetThirdLineAsync(taskQueue, u"", 0);
    }
}

void BannerView::SetGameTitleAsync(TaskQueueBase* taskQueue, const char16_t* gameTitle)
{
    const char16_t* lines[3] = { nullptr, nullptr, nullptr };
    u32 lineLens[3] = { 0, 0, 0 };

    const char16_t* p = gameTitle;
    int lineCount = 0;
    const char16_t* lineStart = p;

    while (true)
    {
        char16_t c = *p;
        if (c == 0 || c == '\n')
        {
            if (lineCount < 3)
            {
                lines[lineCount] = lineStart;
                lineLens[lineCount] = p - lineStart;
                lineCount++;
            }
            if (c == 0)
                break;
            p++;
            lineStart = p;
        }
        else
        {
            p++;
        }
    }

    if (lineCount >= 3)
    {
        _lines = 3;
        SetFirstLineAsync(taskQueue, lines[0], lineLens[0], false);
        SetSecondLineAsync(taskQueue, lines[1], lineLens[1]);
        SetThirdLineAsync(taskQueue, lines[2], lineLens[2]);
    }
    else if (lineCount == 2)
    {
        _lines = 2;
        SetFirstLineAsync(taskQueue, lines[0], lineLens[0], false);
        SetSecondLineAsync(taskQueue, lines[1], lineLens[1]);
        SetThirdLineAsync(taskQueue, u"", 0);
    }
    else if (lineCount == 1)
    {
        _lines = 1;
        SetFirstLineAsync(taskQueue, lines[0], lineLens[0], false);
        SetSecondLineAsync(taskQueue, u"", 0);
        SetThirdLineAsync(taskQueue, u"", 0);
    }
    else
    {
        _lines = 0;
        SetFirstLineAsync(taskQueue, u"", 0, false);
        SetSecondLineAsync(taskQueue, u"", 0);
        SetThirdLineAsync(taskQueue, u"", 0);
    }
}