#pragma once
#include <memory>
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "gui/views/View.h"
#include "../FileType/FileIcon.h"
#include "RomBrowserItemInputHandler.h"

class MaterialColorScheme;
class GraphicsContext;
class VramContext;

class IconGridItemView : public View
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

    class FavoriteBadgeVramToken
    {
        u32 _vramOffset;
    public:
        FavoriteBadgeVramToken()
            : _vramOffset(0) { }

        explicit FavoriteBadgeVramToken(u32 offset)
            : _vramOffset(offset) { }

        constexpr u32 GetVramOffset() const { return _vramOffset; }
    };

    void InitVram(const VramContext& vramContext) override;
    void Update() override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

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

    virtual void SetGraphics(const VramToken& vramToken) { }

    void SetFavoriteBadgeGraphics(const FavoriteBadgeVramToken& vramToken)
    {
        _favoriteBadgeVramOffset = vramToken.GetVramOffset();
    }

    static FavoriteBadgeVramToken UploadFavoriteBadgeGraphics(const VramContext& vramContext);

    RomBrowserItemViewModel& GetViewModel() const
    {
        return *_viewModel;
    }

protected:
    std::unique_ptr<RomBrowserItemViewModel> _viewModel;
    std::unique_ptr<FileIcon> _icon;
    vu16* _iconVram;
    u32 _iconVramOffset;
    u32 _favoriteBadgeVramOffset = 0;
    RomBrowserItemInputHandler _inputHandler;

    explicit IconGridItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel)
        : _viewModel(std::move(viewModel)), _inputHandler(this, _viewModel.get()) { }

    void DrawFavoriteBadge(GraphicsContext& graphicsContext) const;
};
