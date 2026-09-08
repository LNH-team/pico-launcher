#pragma once
#include "AppFavorites.h"

class IFavoritesService
{
public:
    virtual ~IFavoritesService() { }

    virtual AppFavorites& GetFavorites() = 0;
    virtual const AppFavorites& GetFavorites() const = 0;
    virtual void Save() const = 0;
};
