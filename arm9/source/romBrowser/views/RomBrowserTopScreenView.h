#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "BannerView.h"
#include "gui/views/LabelView.h"
#include "../FileType/FileIcon.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"

class RomBrowserViewModel;
class IRomBrowserViewFactory;

class RomBrowserTopScreenView : public ViewContainer
{
    SHARED_ONLY(RomBrowserTopScreenView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

private:
    SharedPtr<RomBrowserViewModel> _viewModel;
    const IThemeFileIconFactory* _themeFileIconFactory;
    SharedPtr<BannerView> _fileInfoView;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;
    bool _iconGraphicsUploaded = false;
    bool _coverGraphicsUploaded = false;
    bool _showCover;
    Point _coverPosition;

    RomBrowserTopScreenView(SharedPtr<RomBrowserViewModel> viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory);
};