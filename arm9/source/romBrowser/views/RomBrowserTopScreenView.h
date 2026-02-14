#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "BannerView.h"
#include "../FileType/FileIcon.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "BgmNowPlayingView.h"

class RomBrowserViewModel;
class IRomBrowserViewFactory;
class IBgmService;
class IFontRepository;
struct MaterialColorScheme;

class RomBrowserTopScreenView : public ViewContainer
{
public:
    RomBrowserTopScreenView(const SharedPtr<RomBrowserViewModel>& viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        const IBgmService* bgmService);

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
    std::unique_ptr<BannerView> _fileInfoView;
    std::unique_ptr<BgmNowPlayingView> _bgmNowPlayingView;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;
    bool _iconGraphicsUploaded = false;
    bool _coverGraphicsUploaded = false;
    bool _showCover;
    const IBgmService* _bgmService;
};