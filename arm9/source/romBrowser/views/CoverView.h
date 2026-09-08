#pragma once
#include <memory>
#include "core/AtomicSharedPtr.h"
#include "gui/views/View.h"
#include "../FileType/FileCover.h"
#include "gui/VBlankTextureLoader.h"
#include "romBrowser/viewModels/IRomBrowserItemViewModel.h"
#include "FavoriteBadge.h"
#include "RomBrowserItemInputHandler.h"

class CoverView : public View
{
    SHARED_ONLY(CoverView)

public:
    ~CoverView() override
    {
        _vblankTextureLoader->CancelLoad(_textureLoadRequest);
    }

    void InitVram(const VramContext& vramContext) override;

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x - (COVER_WIDTH / 2), _position.y - (COVER_HEIGHT / 2), COVER_WIDTH, COVER_HEIGHT);
    }

    void SetCover(SharedPtr<FileCover> cover)
    {
        _vblankTextureLoader->CancelLoad(_textureLoadRequest);
        _cover = std::move(cover);
    }

    void ClearCover()
    {
        _vblankTextureLoader->CancelLoad(_textureLoadRequest);
        _cover.Reset();
    }

    void UploadCoverGraphics();

    void SetFavoriteBadgeGraphics(const FavoriteBadgeVramToken& vramToken)
    {
        _favoriteBadgeRenderer.SetVramToken(vramToken);
    }

    void DrawFavoriteBadge(GraphicsContext& graphicsContext) const;

    void CancelPen() { _inputHandler.CancelPen(); }

    IRomBrowserItemViewModel& GetViewModel() const
    {
        return *_viewModel;
    }

private:
    std::unique_ptr<IRomBrowserItemViewModel> _viewModel;
    VBlankTextureLoader* _vblankTextureLoader;
    AtomicSharedPtr<FileCover> _cover;
    VBlankTextureLoadRequest _textureLoadRequest;
    u32 _texVramOffset = 0;
    u32 _plttVramOffset = 0;
    FavoriteBadgeRenderer _favoriteBadgeRenderer;
    RomBrowserItemInputHandler _inputHandler;

    CoverView(std::unique_ptr<IRomBrowserItemViewModel> viewModel, VBlankTextureLoader* vblankTextureLoader)
        : _viewModel(std::move(viewModel)), _vblankTextureLoader(vblankTextureLoader)
        , _inputHandler(this, _viewModel.get()) { }
};
