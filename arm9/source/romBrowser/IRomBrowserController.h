#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class ICheatRepository;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    virtual void LaunchFile(const FileInfo& fileInfo) = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;

    virtual void Update() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
