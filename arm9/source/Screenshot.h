#pragma once
#include <libtwl/mem/memVram.h>
#include "core/task/TaskQueue.h"
#include "gui/StackVramManager.h"

/// @brief Saves a screen to /_pico/screenshots as a 24bpp BMP, using the
///        display capture unit.
///
/// The capture unit belongs to the main engine and can only ever record what
/// that engine draws, so the two screens are saved in two different ways.
///
/// The bottom screen is what the main engine already draws, so it is captured
/// as it is. Block C is borrowed as the destination because it belongs to the
/// sub engine and taking it away cannot disturb the frame being captured; its
/// contents are copied out first and put back afterwards, since they are the top
/// screen's background and nothing re-uploads them.
///
/// The top screen cannot be captured at all - the sub engine has no capture
/// unit, and no register, mapping or screen swap can change that. What works
/// instead is to make the main engine draw the top screen's picture for a single
/// frame and capture that. Block C is the one block on the DS that either
/// engine's backgrounds can read, and it is where this launcher keeps all of the
/// top screen's background data, so the backgrounds need no copying at all: the
/// same bytes mean the same thing to the other engine. Sprites and background
/// palettes are not so lucky - their blocks have no main engine mapping in
/// hardware - so those are copied, which is a few kilobytes.
///
/// The top screen loses its backgrounds for those frames, since the block
/// holding them is what the capture writes into. The bottom screen used to show
/// the same borrowed state; it is blacked out instead, so what the player sees
/// is a flash. Then everything is put back.
class Screenshot
{
public:
    enum class Screen
    {
        Bottom,
        Top,
    };

    /// The object vram managers are needed to find free space for the top
    /// screen's sprite tiles: nothing already allocated may be overwritten.
    Screenshot(TaskQueueBase* ioTaskQueue,
        const StackVramManager* mainObjVram,
        const StackVramManager* mainObjDialogVram,
        const StackVramManager* subObjVram)
        : _ioTaskQueue(ioTaskQueue)
        , _mainObjVram(mainObjVram)
        , _mainObjDialogVram(mainObjDialogVram)
        , _subObjVram(subObjVram) { }

    /// Nothing should be able to go away mid capture, but if it does the
    /// borrowed blocks go back to the engine they came from.
    ~Screenshot();

    /// @brief Asks for both screens, as one request.
    ///
    /// The capture unit records one frame at a time, so the two are taken a
    /// couple of frames apart and cannot be the same instant. They share a
    /// number in their file names, which is what says they belong together.
    void RequestBothScreens()
    {
        // Two doors to the same room, and only the first shows up in _state:
        // the capture hardware is busy, or the io thread is still holding slots
        // from the last gesture.
        if (_state != State::Idle || IsWritePending())
        {
            ReportBusy();
            return;
        }
        _screen = Screen::Bottom;
        _state = State::Requested;
        _pendingTop = true;
        // Cleared here and not only where it is set, because it is a property
        // of one gesture and the paths that abandon a gesture halfway do not
        // all pass through the place that reads it.
        _topIsSecondHalf = false;
    }

    /// @brief Whether a capture is in flight. Everything that draws through the
    ///        engine being borrowed has to sit out those frames.
    bool IsBusy() const { return _state != State::Idle; }

    /// @brief Whether the main engine is being used to draw the top screen, so
    ///        its own per frame work has to be left alone.
    bool IsMirroringMainEngine() const
    {
        return _screen == Screen::Top && _state != State::Idle;
    }

    /// @brief Sets the capture up for the coming frame. Call from the VBlank
    ///        handler: for the bottom screen as the first thing, so the sprites
    ///        uploaded just after are the ones recorded, and the mirroring for
    ///        the top screen needs the sub sprites already uploaded, so this is
    ///        called at the end too.
    void VBlankBegin();

    /// @brief Hands the sub engine's memory, palettes and objects to the main
    ///        engine when a top screen capture is pending. Returns true when it
    ///        did, which means the views owning write only registers have to
    ///        write theirs to the main engine before ArmMirroredCapture.
    bool MirrorSubEngineIfPending();

    /// @brief Points the main engine at the mirrored setup and starts the
    ///        capture. Call after the views have had their say.
    void ArmMirroredCapture();

    /// @brief Reads the captured frame back and puts the hardware right again.
    ///        Call from the visible period (Update): the copies are far too long
    ///        for the VBlank budget.
    void Update();

    /// @brief How the last finished capture ended.
    enum class Result
    {
        None,
        Saved,
        Failed,
        /// Asked for while the previous one was still being written. Its own
        /// answer because it is not a failure and telling the player nothing
        /// is indistinguishable from the shortcut being broken.
        Busy
    };

    /// @brief Returns the outcome of the last capture that finished writing and
    ///        forgets it, so a caller polling it is told once.
    ///
    /// The io thread is what finishes a capture, so this turns up a few frames
    /// after the shutter rather than with it. That delay is why a confirmation
    /// shown on this cannot land inside the picture it is confirming. It says
    /// nothing about the next one: a message still on screen when the player
    /// asks for another shot is part of that shot, the same as anything else
    /// they can see.
    static Result TakeResult();

private:
    /// The display registers the mirror copies over and puts back - only the
    /// ones that can be READ. Scroll, the affine matrices, the window
    /// rectangles, mosaic and blend fade read back as zero on both engines, so
    /// copying those would mirror nothing and silently drop the cover art. They
    /// are written by value instead.
    ///
    /// Here rather than beside the loops that walk it so that the array it
    /// sizes cannot drift from it. It was a hardcoded count in this header and
    /// a separate table in the cpp, with nothing tying the two together: adding
    /// a register would have written one past the end of the save area, and the
    /// compiler only catches that because the loop bound is a constant it can
    /// see through.
    ///
    /// Master brightness is not in this list even though it can be read. The
    /// blackout it carries has to be the last thing lifted, so it is saved and
    /// put back on its own - see RestoreMainEngine.
    static constexpr u16 kMirroredRegs[] =
    {
        0x08, 0x0A, 0x0C, 0x0E, // BG0-3 control
        0x48, 0x4A,             // window contents
        0x50, 0x52,             // blend control and alpha
    };
    static constexpr u32 kMirroredRegCount =
        sizeof(kMirroredRegs) / sizeof(kMirroredRegs[0]);
    /// How many visible periods to wait for the capture before giving up. The
    /// hardware clears the enable bit at the end of the frame it records, so the
    /// bit is normally already clear on the first look and none of this budget is
    /// spent; the allowance is only here so a dropped frame does not throw a good
    /// capture away.
    static constexpr u32 kCaptureWaitFrames = 8;

    /// @brief Reports that a request arrived while a capture was in flight.
    static void ReportBusy();

    /// @brief Whether a file from an earlier capture is still queued on the io
    ///        thread.
    ///
    /// This exists because `_state` cannot answer it. `_state` returns to Idle
    /// as soon as the pixels are out of vram, frames before the file reaches the
    /// card, and it has to: `IsBusy()` is what stops the top screen from
    /// uploading covers, and holding that for the length of a write would freeze
    /// the cover for about a second per shot.
    ///
    /// So the bound lives here instead. A gesture is accepted only when nothing
    /// of ours is outstanding, and one gesture queues exactly two tasks, which
    /// makes "this class never holds more than two of the shared io slots" a
    /// property of the code rather than of how much heap happens to be free.
    /// That margin is thinner than it looks: the icon grid alone asks for 28 of
    /// the 32 slots in a single frame.
    ///
    /// It is a latch, not a timeout. A task that is queued and never runs -
    /// only reachable by enqueueing after the io thread has been stopped -
    /// leaves the shortcut refusing for the rest of the session. There is
    /// deliberately no recovery: resetting the count on a timer would hand one
    /// slot's accounting to two owners.
    static bool IsWritePending();

    enum class State
    {
        Idle,
        /// Asked for; buffers still to be taken and contents still to be saved.
        Requested,
        /// Saved, waiting for the VBlank that sets the hardware up.
        Arming,
        /// The hardware is writing the frame into the destination block.
        Capturing,
    };

    TaskQueueBase* _ioTaskQueue;
    const StackVramManager* _mainObjVram;
    const StackVramManager* _mainObjDialogVram;
    const StackVramManager* _subObjVram;
    State _state = State::Idle;
    /// Set by RequestBothScreens: the top screen follows once the bottom one is
    /// handed to the io thread. Cleared on every path that gives up, so a
    /// failure does not drag a second one behind it.
    bool _pendingTop = false;
    /// Marks the top capture as the second half of a pair, so its file is given
    /// the number the bottom half already claimed instead of a fresh one.
    bool _topIsSecondHalf = false;
    Screen _screen = Screen::Bottom;

    /// What was in the block the capture writes into, and the captured frame.
    /// Both are owned here until the write task takes the pixels.
    u16* _blockBackup = nullptr;
    u16* _pixels = nullptr;

    // Everything the top screen capture has to put back.
    MemVramABMapping _savedVramAMapping = MEM_VRAM_AB_LCDC;
    MemVramCMapping _savedVramCMapping = MEM_VRAM_C_LCDC;
    MemVramFGMapping _savedVramFMapping = MEM_VRAM_FG_LCDC;
    MemVramFGMapping _savedVramGMapping = MEM_VRAM_FG_LCDC;
    MemVramHMapping _savedVramHMapping = MEM_VRAM_H_LCDC;
    u32 _savedDispCnt = 0;
    /// Kept apart from _savedRegs because the blackout is lifted last.
    u16 _savedMasterBright = 0;
    u16 _savedRegs[kMirroredRegCount] = { };
    u16 _savedBgPltt[256] = { };
    u16 _savedObjPltt[256] = { };
    /// The extended palette copies land on blocks F and G, which are the main
    /// engine's background tiles - and the bottom sheet's tiles live there in
    /// the only copy there is, so what is written over has to come back.
    ///
    /// Cache line aligned because these two are the only members filled by dma,
    /// so they are the only ones the cache has to be dropped for first. An
    /// unaligned invalidate throws away the lines at both ends of the range as
    /// well, and the neighbours here are the tail of _savedObjPltt and the two
    /// members below - losing a dirty line there would restore the wrong object
    /// palette, hide the sprites, or leave the vcount irq masked for the rest of
    /// the session. Their size is already a whole number of lines.
    alignas(32) u16 _savedFBytes[256] = { };
    alignas(32) u16 _savedGBytes[256] = { };
    u32 _spriteTileOffset = 0;
    bool _vcountIrqSuppressed = false;
    /// Visible periods spent waiting for the hardware to clear the enable bit.
    u32 _captureWaitFrames = 0;

    void PrepareTopCapture();
    void MirrorSubEngine();
    void RestoreMainEngine();
};
