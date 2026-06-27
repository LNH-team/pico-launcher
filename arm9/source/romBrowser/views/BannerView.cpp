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
    const char16_t* p = gameTitle;
    int i = 0;
    while (true)
    {
        u16 c = *p++;
        if (c == 0 || c == '\n')
        {
            if (i == 0)
            {
                _lines = 1;
                SetFirstLineAsync(taskQueue, gameTitle, p - gameTitle, false);
            }
            else if (i == 1)
            {
                _lines = 2;
                SetSecondLineAsync(taskQueue, gameTitle, p - gameTitle);
            }
            else if (i == 2)
            {
                _lines = 3;
                SetThirdLineAsync(taskQueue, gameTitle, p - gameTitle);
            }
            gameTitle = p;
            i++;
            if (c == 0 || i == 3)
                break;
        }
    }
    if (i <= 1)
        SetSecondLineAsync(taskQueue, u"", 0);
    if (i <= 2)
        SetThirdLineAsync(taskQueue, u"", 0);
}