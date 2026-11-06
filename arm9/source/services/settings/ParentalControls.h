#pragma once

/// @brief What parental controls take away: the actions that can destroy work.
///
/// Only these three, and only while the controls are on. Favourites and the
/// completed marks are never blocked - they change nothing that cannot be put
/// back - and neither is anything else in the launcher.
enum class ParentalFeature
{
    /// @brief Deleting a game and its save.
    Delete,
    /// @brief The cheats sheet.
    Cheats,
    /// @brief Renaming a game: its name, developer and publisher.
    GameInfo
};

/// @brief Whether the destructive actions are locked.
///
/// There is no launcher screen for this: it is one flag in settings.json,
/// edited on a PC, which is also the way back out - set it to false and the
/// lock is off, with nothing to remember and nothing to forget.
///
/// There is nothing else to configure: no per-feature switches, and every
/// blocking decision is made in RomBrowserController, so a blocked action
/// cannot be reached by another route.
class ParentalControls
{
public:
    bool enabled = false;

    /// @brief Whether a feature is available right now.
    constexpr bool Allows(ParentalFeature feature) const
    {
        if (!enabled)
            return true;
        switch (feature)
        {
            case ParentalFeature::Delete:
            case ParentalFeature::Cheats:
            case ParentalFeature::GameInfo:
                return false;
            default:
                return true;
        }
    }
};
