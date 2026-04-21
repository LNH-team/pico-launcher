#pragma once
#include "core/AtomicSharedPtr.h"
#include "gui/views/View.h"
#include "romBrowser/FileType/FileCover.h"
#include "gui/VBlankTextureLoader.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "romBrowser/views/RomBrowserItemInputHandler.h"

class MaterialCoverView : public View
{
    SHARED_ONLY(MaterialCoverView)

public:
    ~MaterialCoverView() override
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
        return Rectangle(_position.x, _position.y, COVER_WIDTH, COVER_HEIGHT);
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

    RomBrowserItemViewModel& GetViewModel() const
    {
        return *_viewModel;
    }

private:
    std::unique_ptr<RomBrowserItemViewModel> _viewModel;
    VBlankTextureLoader* _vblankTextureLoader;
    AtomicSharedPtr<FileCover> _cover;
    VBlankTextureLoadRequest _textureLoadRequest;
    u32 _texVramOffset = 0;
    u32 _plttVramOffset = 0;
    RomBrowserItemInputHandler _inputHandler;

    MaterialCoverView(std::unique_ptr<RomBrowserItemViewModel> viewModel, VBlankTextureLoader* vblankTextureLoader)
        : _viewModel(std::move(viewModel)), _vblankTextureLoader(vblankTextureLoader)
        , _inputHandler(this, _viewModel.get()) { }
};
