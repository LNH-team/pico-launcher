#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "services/settings/KeyCombo.h"
#include "ExitComboBottomSheetView.h"

// Placed from the sheet's top edge, which rests at y 32 once the sheet is open.
#define TITLE_X             20
#define TITLE_Y             16

#define COMBO_X             20
#define COMBO_Y             44
#define COMBO_WIDTH         216

#define STATUS_X            20
#define STATUS_Y            62
#define STATUS_WIDTH        216

// The hints: the button on the left in the heavier face, what it does beside
// it. Three rows end at 144, screen y 176, inside the sheet's rounded edge.
#define HINT_BUTTON_X       20
#define HINT_BUTTON_WIDTH   40
#define HINT_DESCRIPTION_X  64
#define HINT_DESCRIPTION_WIDTH  176
#define HINT_Y              92
#define HINT_STEP           18

ExitComboBottomSheetView::ExitComboBottomSheetView(SharedPtr<ExitComboViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel = AddLabel(fontRepository, FontType::Medium11, 160, 20, "In-game exit", Alignment::Start);
    _comboLabel = AddLabel(fontRepository, FontType::Medium10, COMBO_WIDTH, 26, "", Alignment::Start);
    _statusLabel = AddLabel(fontRepository, FontType::Regular10, STATUS_WIDTH, 40, "", Alignment::Start);
    for (auto& label : _hintButtons)
        label = AddLabel(fontRepository, FontType::Medium10, HINT_BUTTON_WIDTH, 8, "", Alignment::Start);
    for (auto& label : _hintDescriptions)
        label = AddLabel(fontRepository, FontType::Regular10, HINT_DESCRIPTION_WIDTH, 40, "", Alignment::Start);
    UpdateText();
}

SharedPtr<Label2DView> ExitComboBottomSheetView::AddLabel(const IFontRepository* fontRepository,
    FontType fontType, u32 width, u32 maxChars, const char* text, Alignment alignment)
{
    auto label = Label2DView::CreateShared(width, 16, maxChars, fontRepository->GetFont(fontType));
    label->SetHorizontalAlignment(alignment);
    label->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    label->SetText(text);
    AddChildTail(label.GetPointer());
    return label;
}

void ExitComboBottomSheetView::UpdateText()
{
    char text[KeyCombo::kMaxTextLength];
    if (_capturing)
    {
        // Zero would format as the default, and nothing is held yet.
        if (_heldKeys != 0)
            KeyCombo::Format(_heldKeys, text, sizeof(text));
        else
            text[0] = 0;
        _comboLabel->SetText(text);

        u32 count = KeyCombo::Count(_heldKeys);
        const char* status;
        if (_heldKeys == 0)
            status = "Hold 2 to 4 keys, SELECT included";
        else if ((_heldKeys & static_cast<u16>(InputKey::Select)) == 0)
            status = "SELECT must be one of them";
        else if (count < 2)
            status = "One more key";
        else if (count > 4)
            status = "Too many keys";
        else if (count == 2)
            status = "Keep holding - 2 keys is easy to hit";
        else
            status = "Keep holding for a moment";
        _statusLabel->SetText(status);

        _hintButtons[0]->SetText("B");
        _hintDescriptions[0]->SetText("cancel");
        _hintButtons[1]->SetText("");
        _hintDescriptions[1]->SetText("");
        _hintButtons[2]->SetText("");
        _hintDescriptions[2]->SetText("");
    }
    else
    {
        u16 combo = _viewModel->GetCombo();
        KeyCombo::Format(combo, text, sizeof(text));
        _comboLabel->SetText(text);
        _statusLabel->SetText(combo == 0 ? "the loader's default" : "");

        _hintButtons[0]->SetText("A");
        _hintDescriptions[0]->SetText("pick a new combination");
        _hintButtons[1]->SetText("Y");
        _hintDescriptions[1]->SetText("use the loader's default");
        _hintButtons[2]->SetText("B");
        _hintDescriptions[2]->SetText("close");
    }
}

void ExitComboBottomSheetView::Update()
{
    if (_capturing)
    {
        if (_heldKeys != _steadyKeys)
        {
            _steadyKeys = _heldKeys;
            _steadyFrames = 0;
        }
        else if (_steadyFrames < kHoldFrames)
        {
            _steadyFrames++;
        }
        if (_steadyFrames >= kHoldFrames && KeyCombo::IsValid(_steadyKeys))
        {
            // The loader checks the same rules again; this only spares the
            // player from storing something it would refuse.
            _viewModel->SetCombo(_steadyKeys);
            _capturing = false;
            _heldKeys = 0;
            _steadyKeys = 0;
            _steadyFrames = 0;
            UpdateText();
        }
    }

    int y = _position.y;
    _titleLabel->SetPosition(TITLE_X, y + TITLE_Y);
    _comboLabel->SetPosition(COMBO_X, y + COMBO_Y);
    _statusLabel->SetPosition(STATUS_X, y + STATUS_Y);
    for (int i = 0; i < 3; i++)
    {
        int rowY = y + HINT_Y + i * HINT_STEP;
        _hintButtons[i]->SetPosition(HINT_BUTTON_X, rowY);
        _hintDescriptions[i]->SetPosition(HINT_DESCRIPTION_X, rowY);
    }
    BottomSheetView::Update();
}

void ExitComboBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        const auto back = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        _titleLabel->SetBackgroundColor(back);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _comboLabel->SetBackgroundColor(back);
        _comboLabel->SetForegroundColor(_materialColorScheme->primary);
        _statusLabel->SetBackgroundColor(back);
        _statusLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        for (auto& label : _hintButtons)
        {
            label->SetBackgroundColor(back);
            label->SetForegroundColor(_materialColorScheme->onSurface);
        }
        for (auto& label : _hintDescriptions)
        {
            label->SetBackgroundColor(back);
            label->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        }
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool ExitComboBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (_capturing)
    {
        // A lone B cancels; with SELECT held it is a key like any other, so a
        // combo may contain it. Everything is swallowed, the sheet is capturing.
        if (inputProvider.Triggered(InputKey::B) &&
            !inputProvider.Current(InputKey::Select))
        {
            _capturing = false;
            _heldKeys = 0;
            _steadyKeys = 0;
            _steadyFrames = 0;
            UpdateText();
            return true;
        }
        u16 heldKeys = static_cast<u16>(inputProvider.GetCurrentKeys()) & KeyCombo::kValidKeys;
        if (heldKeys != _heldKeys)
        {
            _heldKeys = heldKeys;
            UpdateText();
        }
        return true;
    }

    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    if (inputProvider.Triggered(InputKey::A))
    {
        _capturing = true;
        _heldKeys = 0;
        _steadyKeys = 0;
        _steadyFrames = 0;
        UpdateText();
        return true;
    }
    if (inputProvider.Triggered(InputKey::Y))
    {
        _viewModel->SetCombo(0);
        UpdateText();
        return true;
    }
    return false;
}

SharedPtr<View> ExitComboBottomSheetView::MoveFocus(
    const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    // Nothing takes focus. Keeping the d-pad here also keeps it captured: the
    // focus manager sees the press as a move and never sends it as a key.
    return nullptr;
}

void ExitComboBottomSheetView::Focus(FocusManager& focusManager)
{
    // a child of the sheet, so keys reach it and bubble up to HandleInput
    focusManager.Focus(_titleLabel->SharedFromThis());
}

void ExitComboBottomSheetView::Close()
{
    _viewModel->Close();
}
