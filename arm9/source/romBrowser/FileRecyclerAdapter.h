#pragma once
#include "core/task/TaskQueue.h"
#include "gui/views/RecyclerAdapter.h"

class FileInfoManager;
class IVramManager;
class InternalFileInfo;
class IThemeFileIconFactory;
class VramContext;
class IRomBrowserController;

class FileRecyclerAdapter : public RecyclerAdapter
{
public:
    u32 GetItemCount() const override;
    void BindView(SharedPtr<View> view, int index) const override;

    void SetIconFrameCounter(u32 iconFrameCounter)
    {
        _iconFrameCounter = iconFrameCounter;
    }

    virtual void InitVram(const VramContext& vramContext) { }

protected:
    IRomBrowserController* _romBrowserController;
    FileInfoManager* _fileInfoManager;
    TaskQueueBase* _taskQueue;
    u32 _iconFrameCounter;
    const IThemeFileIconFactory* _themeFileIconFactory;

    FileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory)
        : _romBrowserController(romBrowserController), _fileInfoManager(fileInfoManager), _taskQueue(taskQueue)
        , _iconFrameCounter(0), _themeFileIconFactory(themeFileIconFactory) { }

    virtual TaskResult<void> BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const = 0;
    virtual void SetQueueTask(const SharedPtr<View>& view, QueueTask<void> queueTask) const = 0;
};
