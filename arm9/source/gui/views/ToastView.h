#pragma once
#include "common.h"
#include "animation/Animator.h"
#include "core/SharedPtr.h"
#include "gui/views/Label3DView.h"
#include "gui/views/View.h"
#include "themes/IFontRepository.h"
#include "themes/material/MaterialColorScheme.h"

class VBlankTextureLoader;

/// @brief A short message that confirms something happened and then goes away
///        on its own.
///
/// The only other way the launcher speaks to the user is the bottom sheet, which
/// is modal: it waits for an answer. This one asks for nothing. It never takes
/// focus, never takes input, and leaves whether it was read or not, so it suits
/// the things that need saying but not answering - a screenshot written, a jump
/// that landed on a letter, a save that failed.
///
/// It is dressed in the colours the active theme gives its cards and it is sized
/// to its own text, so it reads as part of the page rather than as a panel laid
/// over it. It draws on the bottom screen, the one the user is looking at and the
/// one the launcher fully owns.
class alignas(32) ToastView : public View
{
    SHARED_ONLY(ToastView)

public:
    /// @brief Shows the given message, restarting the timer if one is already up.
    ///
    /// Only one message exists at a time and the newest wins. A queue would make
    /// the user wait to be told about the thing they just did, and the older
    /// message is the one they care about least.
    void Show(const char* text);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;
    Rectangle GetBounds() const override;

private:
    enum class State
    {
        Hidden,
        FadingIn,
        Holding,
        FadingOut
    };

    ToastView(const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository, VBlankTextureLoader* vblankTextureLoader);

    /// @brief Draws one horizontal band of the panel, inset from both edges by
    ///        the same amount. Stacking bands is what rounds the corners.
    void DrawRow(int y, int height, int inset, int depth) const;

    /// @brief Hides the sprites behind the panel, and puts them back.
    ///
    /// The 3d layer this is drawn on is background 0, which the launcher gives
    /// the lowest priority there is, and a sprite beats a background of equal
    /// priority. So in any layout that fills the screen with sprites - the icon
    /// grid does - the panel would sit behind them. Window 1 of the main engine
    /// is unused, and a window can switch objects off inside its rectangle,
    /// which is the only way to be in front of them without rebuilding this as
    /// a background layer of its own.
    ///
    /// Decided in Draw and applied in VBlank, because the panel is geometry:
    /// what Draw submits only reaches the screen after the buffer swap, while a
    /// register write lands on the frame being scanned right now. Writing the
    /// window from Draw therefore opened it a frame EARLY - the first frame of
    /// a message switched the sprites off with nothing drawn over them yet -
    /// and closed it a frame early too, putting them back while the last panel
    /// was still on screen. The class exists to avoid exactly that rectangle of
    /// hidden sprites with nothing on top; it was guarding one edge of it.
    ///
    /// Not from Update either: that runs in the visible period, when a capture
    /// of the top screen may already have borrowed this engine.
    void ApplyWindow();
    void DisableWindow();

public:
    /// @brief Asks for the window to be closed, for the frames a caller has to
    ///        keep this off the screen.
    ///
    /// Skipping Draw is not enough on its own: the window would stay open with
    /// the last rectangle and nothing painted inside it, which is the hole this
    /// class exists to avoid. This says so instead, and the next VBlank acts on
    /// it - which is also why it writes no register: the callers that need this
    /// are the ones whose frames the engine has been lent out on.
    void Suppress() { _wantWindow = false; }

private:

    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label3DView> _label;
    State _state = State::Hidden;
    u32 _holdFrames = 0;
    /// Drives the fade and the rise together, so they cannot come apart.
    Animator<int> _progressAnimator;
    int _x = 0;
    int _y = 0;
    u32 _width = 0;
    u32 _alpha = 0;
    /// What Draw decided the window should be doing, for VBlank to carry out.
    bool _wantWindow = false;
};
