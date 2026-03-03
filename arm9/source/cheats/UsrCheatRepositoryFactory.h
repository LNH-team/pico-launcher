#pragma once
#include <memory>
#include "UsrCheatRepository.h"

class UsrCheatRepositoryFactory
{
public:
    std::unique_ptr<UsrCheatRepository> FromUsrCheatDat(const TCHAR* usrCheatDatPath);
};
