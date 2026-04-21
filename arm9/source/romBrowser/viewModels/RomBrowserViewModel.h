#pragma once
#include <memory>
#include "core/task/TaskQueue.h"
#include "../SdFolder.h"
#include "../FileInfoManager.h"
#include "../IRomBrowserController.h"

class ICoverRepository;

/// @brief View model for the rom browser
class RomBrowserViewModel
{
public:
    RomBrowserViewModel(IRomBrowserController* romBrowserController, const char* initialSelectedFileName = nullptr);

    IRomBrowserController* GetRomBrowserController() const { return _romBrowserController; }
    FileInfoManager& GetFileInfoManager() const { return *_fileInfoManager; }
    TaskQueueBase* GetIoTaskQueue() const { return _romBrowserController->GetIoTaskQueue(); }
    TaskQueueBase* GetBgTaskQueue() const { return _romBrowserController->GetBgTaskQueue(); }
    const ICoverRepository& GetCoverRepository() const { return _romBrowserController->GetCoverRepository(); }

    constexpr int GetSelectedItem() const { return _selectedItem; }
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    constexpr u32 GetIconFrameCounter() const { return _iconFrameCounter; }
    void SetIconFrameCounter(u32 iconFrameCounter) { _iconFrameCounter = iconFrameCounter; }

    void NavigateUp();

private:
    IRomBrowserController* _romBrowserController;
    std::unique_ptr<FileInfoManager> _fileInfoManager;
    int _selectedItem = -1;
    u32 _iconFrameCounter = 0;
};
