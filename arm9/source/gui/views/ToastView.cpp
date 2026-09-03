#include "common.h"
#include <algorithm>
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include <libtwl/gfx/gfxWindow.h>
#include <libtwl/gfx/gfx.h>
#include "gui/materialDesign.h"
#include "themes/FontType.h"
#include "ToastView.h"

// Low on the screen, clear of the carousel covers that run from y 56 to y 152,
// and clear of the app bar, which every layout puts at the top or down the left
// side. Being in front is handled by depth rather than by hiding here, so this is
// a placement choice and moving it is one number.
#define TOAST_BOTTOM        180
#define TOAST_HEIGHT        22

// Room to breathe on both sides of the text. The panel is sized to the string
// rather than to the screen, so a two word confirmation reads as something the
// launcher put there on purpose instead of a bar across the bottom.
#define TEXT_PADDING_X      14
#define MIN_WIDTH           72
#define MAX_WIDTH           232

#define TEXT_HEIGHT         12
#define TEXT_WIDTH          (MAX_WIDTH - 2 * TEXT_PADDING_X)

// The label rounds its texture up to the next power of two, so asking for more
// than 256 pixels of width would double the vram it holds for the session.
#define MAX_STRING_LENGTH   48

// One pixel off each corner, and no more than one.
//
// The window that hides the sprites underneath can only be a rectangle, so
// every pixel of that rectangle the panel does not fill shows the bare page
// instead - no icons, no panel. Measured on a real capture with a radius of
// four: the corners came out as the page colour, which against a game icon
// reads as four white notches, and on a themed background would be whatever
// that theme happens to have there. That is a shape mismatch, not a colour
// problem, so the fix is to stop mismatching.
//
// At one pixel the panel is a rectangle with its corners shaved: enough to not
// read as a hard box, and small enough that what shows through cannot be seen
// on any theme. Rounding it properly needs the window to have the same shape as
// the panel, which means an object window and a mask sprite - worth doing if
// this ever wants to be a real pill, and not worth it for four pixels.
#define CORNER_ROWS         1
static const u8 kCornerInset[CORNER_ROWS] = { 1 };

// Material calls this pattern a snackbar and gives its short form four seconds.
// Two is enough here: the user caused the thing being confirmed, so they are
// already looking, and it sits over the list they are using.
#define HOLD_FRAMES         120
#define FADE_IN_FRAMES      md::sys::motion::duration::medium2
#define FADE_OUT_FRAMES     md::sys::motion::duration::short4

// How far it starts below where it settles. Small on purpose: this should read
// as the panel arriving, not as something sliding across the screen.
#define RISE_PIXELS         4

// The text has to be nearer than the panel it sits on, and the panel nearer than
// anything it covers. Nearer than everything, in fact: the custom themes draw
// their icon grid items at minus five, so a confirmation sitting at zero would be
// hidden behind the grid on those themes. Just past that rather than far past it,
// because a vertex in front of the near plane is clipped away entirely.
#define BACKGROUND_DEPTH    (-6)
#define TEXT_DEPTH          (-7)

#define MAX_ALPHA           31
#define PROGRESS_MAX        256

// Two ids, because translucent polygons that share one do not blend with each
// other: on a single id the text would punch a hole through the panel while both
// are fading. And not 62 or 63, which look free and are not - the icon button
// selector draws with 62 and the custom app bar scrim with 63, so a shared id
// with either would drop the overlap instead of blending it. Everything from 1
// to 61 is unused.
#define BACKGROUND_POLYGON_ID   60
#define TEXT_POLYGON_ID         61

ToastView::ToastView(const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, VBlankTextureLoader* vblankTextureLoader)
    : _materialColorScheme(materialColorScheme)
    , _label(Label3DView::CreateShared(TEXT_WIDTH, TEXT_HEIGHT, MAX_STRING_LENGTH,
        fontRepository->GetFont(FontType::Medium10), vblankTextureLoader))
    , _progressAnimator(0)
{
    _label->SetDepth(TEXT_DEPTH);
    _label->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
}

void ToastView::InitVram(const VramContext& vramContext)
{
    _label->InitVram(vramContext);
}

void ToastView::Show(const char* text)
{
    _label->SetText(text);

    // Sized to the string now that there is one, and centred on the screen.
    u32 textWidth = _label->GetStringWidth();
    _width = std::clamp(textWidth + 2 * TEXT_PADDING_X, (u32)MIN_WIDTH, (u32)MAX_WIDTH);
    _x = (256 - (int)_width) / 2;

    // Restarting from whatever is on screen rather than from nothing, so a second
    // message replacing a first one does not blink.
    _state = State::FadingIn;
    _holdFrames = 0;
    _progressAnimator.Goto(PROGRESS_MAX, FADE_IN_FRAMES, &md::sys::motion::easing::emphasizedDecelerate);
}

void ToastView::Update()
{
    if (_state == State::Hidden)
        return;

    switch (_state)
    {
        case State::FadingIn:
        {
            if (_progressAnimator.Update())
                _state = State::Holding;
            break;
        }

        case State::Holding:
        {
            if (++_holdFrames >= HOLD_FRAMES)
            {
                _state = State::FadingOut;
                _progressAnimator.Goto(0, FADE_OUT_FRAMES, &md::sys::motion::easing::emphasizedAccelerate);
            }
            break;
        }

        case State::FadingOut:
        {
            if (_progressAnimator.Update())
                _state = State::Hidden;
            break;
        }

        case State::Hidden:
            break;
    }

    int progress = _progressAnimator.GetValue();
    _alpha = (u32)(progress * MAX_ALPHA / PROGRESS_MAX);
    // Rises into place as it appears, and sinks back as it goes.
    _y = TOAST_BOTTOM - TOAST_HEIGHT + RISE_PIXELS - progress * RISE_PIXELS / PROGRESS_MAX;

    _label->SetAlpha(_alpha);
    _label->SetPosition(_x + TEXT_PADDING_X, _y + (TOAST_HEIGHT - TEXT_HEIGHT) / 2);
    _label->Update();
}

Rectangle ToastView::GetBounds() const
{
    return Rectangle(_x, _y, (int)_width, TOAST_HEIGHT);
}

// Bit per layer in the window contents registers: backgrounds 0 to 3, then
// objects, then the colour special effects.
#define WINDOW_ALL_BGS      0x0F
#define WINDOW_OBJ          0x10
#define WINDOW_EFFECTS      0x20
#define WINDOW_EVERYTHING   (WINDOW_ALL_BGS | WINDOW_OBJ | WINDOW_EFFECTS)
#define DISPCNT_WINDOW_1    (1 << 14)

// Backgrounds 1 and 2, which the bottom sheet owns: the sheet's panel is
// background 1 at priority 1 and its scrim is background 2 at priority 2, while
// this is drawn on the 3d layer, which is background 0 at priority 3. Both of
// them therefore beat it, and switching the objects off does nothing about a
// background - so with a sheet open the message was drawn underneath it and the
// user was told nothing at all.
//
// A window can only switch a layer off, so the sheet is switched off inside this
// rectangle rather than the message being lifted above it. While the message is
// fading its panel is not yet opaque, so for those frames the rectangle shows
// the page behind the sheet instead of the sheet. That is the price of the only
// mechanism the hardware offers here, and it buys a confirmation that is
// actually visible.
#define WINDOW_SHEET_BGS    0x06

void ToastView::ApplyWindow()
{
    gfx_setWindow1((u8)_x, (u8)_y, (u8)(_x + (int)_width), (u8)(_y + TOAST_HEIGHT));

    // Only the top half of this register belongs to window 1; window 0 is the
    // screenshot's, so its half is left exactly as it was.
    REG_WININ = (u16)((REG_WININ & 0x00FF) |
        (((WINDOW_ALL_BGS & ~WINDOW_SHEET_BGS) | WINDOW_EFFECTS) << 8));

    // Enabling any window makes the whole screen outside it obey this, so it has
    // to say everything, or turning the toast on would change the rest of the
    // screen. The object window half is left alone.
    REG_WINOUT = (u16)((REG_WINOUT & 0xFF00) | WINDOW_EVERYTHING);

    REG_DISPCNT |= DISPCNT_WINDOW_1;
}

void ToastView::DisableWindow()
{
    // Unconditional, not guarded by a flag of our own. A screenshot of the top
    // screen saves and restores the whole display control register, so if the
    // toast happens to end during those frames the window comes back enabled
    // with a stale rectangle. A flag would then think it had already been
    // switched off and leave a patch of hidden sprites on screen for good.
    REG_DISPCNT &= ~DISPCNT_WINDOW_1;
}

void ToastView::VBlank()
{
    // Unconditional both ways, and no flag of our own to remember what the
    // hardware was last told: a top screen capture saves and restores the whole
    // display control register, so a window this class thought it had already
    // switched off can come back enabled with a stale rectangle.
    if (_wantWindow)
        ApplyWindow();
    else
        DisableWindow();
}

void ToastView::DrawRow(int y, int height, int inset, int depth) const
{
    int left = _x + inset;
    int right = _x + (int)_width - inset;
    REG_GX_VTX_16 = GX_VTX_PACK(left << 6, y << 3);
    REG_GX_VTX_16 = depth << 6;
    REG_GX_VTX_16 = GX_VTX_PACK(left << 6, (y + height) << 3);
    REG_GX_VTX_16 = depth << 6;
    REG_GX_VTX_16 = GX_VTX_PACK(right << 6, (y + height) << 3);
    REG_GX_VTX_16 = depth << 6;
    REG_GX_VTX_16 = GX_VTX_PACK(right << 6, y << 3);
    REG_GX_VTX_16 = depth << 6;
}

void ToastView::Draw(GraphicsContext& graphicsContext)
{
    // The window has to follow exactly what gets drawn, so the decision is
    // made here, next to the drawing, and carried out in VBlank so that it
    // lands on the same frame the panel does. Deciding it anywhere else went
    // wrong twice: from Update it wrote the registers of an engine a capture
    // had already borrowed, and on the frame a fade finished it enabled the
    // window while Draw bailed out on zero alpha, leaving a hole with nothing
    // drawn over it.
    if (_state == State::Hidden || _alpha == 0 || !graphicsContext.IsVisible(GetBounds()))
    {
        _wantWindow = false;
        return;
    }

    _wantWindow = true;

    // No texture at all, so the panel costs nothing in vram and does not depend
    // on a texel that only one theme happens to keep around. With texturing off
    // in modulate mode, the vertex colour is what gets drawn.
    //
    // The colours are the pair the launcher already dresses its own cards in,
    // taken from the active theme, so this belongs to the page rather than
    // sitting on top of it. Material would invert a snackbar to its darkest
    // surface, which here would read as a foreign element.
    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, _alpha, BACKGROUND_POLYGON_ID);
    Gx::TexImageParam(0, false, false, false, false, GX_TEXSIZE_8, GX_TEXSIZE_8,
        GX_TEXFMT_NONE, false, GX_TEXGEN_NONE);
    Gx::Color(Rgb<5, 5, 5>(_materialColorScheme->secondaryContainer));

    Gx::Begin(GX_PRIMITIVE_QUAD);
    for (int i = 0; i < CORNER_ROWS; i++)
        DrawRow(_y + i, 1, kCornerInset[i], BACKGROUND_DEPTH);
    DrawRow(_y + CORNER_ROWS, TOAST_HEIGHT - 2 * CORNER_ROWS, 0, BACKGROUND_DEPTH);
    for (int i = 0; i < CORNER_ROWS; i++)
        DrawRow(_y + TOAST_HEIGHT - 1 - i, 1, kCornerInset[i], BACKGROUND_DEPTH);
    Gx::End();

    _label->SetForegroundColor(_materialColorScheme->onSecondaryContainer);
    u32 oldPolygonId = graphicsContext.SetPolygonId(TEXT_POLYGON_ID);
    _label->Draw(graphicsContext);
    graphicsContext.SetPolygonId(oldPolygonId);
}
