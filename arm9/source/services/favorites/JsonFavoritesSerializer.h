#pragma once
class AppFavorites;

enum class FavoritesDeserializeResult
{
    Success,
    NotFound,
    Error
};

class JsonFavoritesSerializer
{
public:
    void Serialize(const AppFavorites* appFavorites, const char* filePath) const;
    FavoritesDeserializeResult Deserialize(AppFavorites* appFavorites, const char* filePath) const;
};
