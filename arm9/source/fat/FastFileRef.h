#pragma once
#include "ff.h"

class FastFileRef
{
    FATFS* _fatFs;
    u32 _dirSector;
    u32 _dirSectorOffset;
    u32 _startCluster;
    FSIZE_t _fileSize;

public:
    FastFileRef() { }

    explicit FastFileRef(const DIR* directory, const FILINFO* fileInfo)
        : _fatFs(directory->obj.fs), _dirSector(fileInfo->fdirsect)
        , _dirSectorOffset(fileInfo->fdiroffs), _startCluster(fileInfo->fclust)
        , _fileSize(fileInfo->fsize) { }

    FastFileRef(FATFS* fs, const FILINFO* fileInfo)
        : _fatFs(fs), _dirSector(fileInfo->fdirsect)
        , _dirSectorOffset(fileInfo->fdiroffs), _startCluster(fileInfo->fclust)
        , _fileSize(fileInfo->fsize) { }

    FATFS* GetFatFs() const { return _fatFs; }
    u32 GetDirSector() const { return _dirSector; }
    u32 GetDirSectorOffset() const { return _dirSectorOffset; }
    u32 GetStartCluster() const { return _startCluster; }
    FSIZE_t GetFileSize() const { return _fileSize; }
};