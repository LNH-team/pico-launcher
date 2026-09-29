#pragma once
#include <memory>
#include "core/String.h"

/// @brief Favorites data: an array of favorited items' full file paths, and the current
/// count. Lives at /_pico/favorites.json - see JsonFavoritesSerializer. Kept separate from
/// AppSettings so a build that doesn't know about favorites can never silently wipe them
/// out just by saving its own settings.
class AppFavorites
{
public:
    std::unique_ptr<String<char, 256>[]> favorites;
    u32 numberOfFavorites = 0;
};
