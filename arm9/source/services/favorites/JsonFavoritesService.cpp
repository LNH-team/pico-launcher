#include "common.h"
#include "JsonFavoritesService.h"

#define FAVORITES_FILE_PATH  "/_pico/favorites.json"

JsonFavoritesService::JsonFavoritesService()
{
    switch (_serializer.Deserialize(&_appFavorites, FAVORITES_FILE_PATH))
    {
        case FavoritesDeserializeResult::NotFound:
        {
            // No file (or an empty one) is the normal first run: there is nothing to lose,
            // so write the defaults and leave saving enabled.
            Save();
            break;
        }
        case FavoritesDeserializeResult::Error:
        {
            // The file is there but unreadable - corruption, a bad manual edit, a card
            // error. Leave it alone and latch saving off, see Save().
            _loadFailed = true;
            break;
        }
        case FavoritesDeserializeResult::Success:
        {
            break;
        }
    }
}

void JsonFavoritesService::Save() const
{
    // The file existed but this run could not read it, so memory holds none of what is on
    // the card. Save() rewrites the whole file, so writing now would replace every favorite
    // with whatever this session happened to add. Losing this session's changes is the
    // lesser loss, and the file stays recoverable by hand.
    if (_loadFailed)
    {
        LOG_ERROR("Favorites not saved: the existing file failed to load\n");
        return;
    }

    _serializer.Serialize(&_appFavorites, FAVORITES_FILE_PATH);
}
