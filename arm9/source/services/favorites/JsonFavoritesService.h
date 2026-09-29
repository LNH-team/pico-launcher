#pragma once
#include "IFavoritesService.h"
#include "JsonFavoritesSerializer.h"

/// @brief Owns the favorites array in memory and persists it to its own dedicated file,
/// /_pico/favorites.json, completely separate from settings.json. This isolation is the
/// whole point: a build whose AppSettings doesn't know about favorites can no longer
/// silently wipe them out just by saving its own settings.
class JsonFavoritesService : public IFavoritesService
{
public:
    JsonFavoritesService();

    AppFavorites& GetFavorites() override { return _appFavorites; }
    const AppFavorites& GetFavorites() const override { return _appFavorites; }

    /// @brief Writes the whole favorites file, unless this run failed to read it - see the
    ///        comment in the implementation.
    void Save() const override;

private:
    JsonFavoritesSerializer _serializer;
    AppFavorites _appFavorites;
    /// @brief Set when the file existed but could not be read or parsed. Latches saving off
    ///        for the rest of the run.
    bool _loadFailed = false;
};
