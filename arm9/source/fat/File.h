#pragma once
#include "ff.h"
#include "FastFileRef.h"

class alignas(32) File
{
    FIL _file;

public:
    FRESULT Open(const TCHAR* path, BYTE mode) { return f_open(&_file, path, mode); }

        FRESULT Open(const FastFileRef& fastFileRef, BYTE mode)
        {
        _file.obj.fs = fastFileRef.GetFatFs();
        _file.obj.id = fastFileRef.GetFatFs()->id;
        _file.obj.sclust = fastFileRef.GetStartCluster();
        _file.obj.attr = 0;
        _file.obj.objsize = fastFileRef.GetFileSize();
        _file.dir_sect = fastFileRef.GetDirSector();
        _file.dir_ptr = &fastFileRef.GetFatFs()->win[fastFileRef.GetDirSectorOffset()];
    #if FF_USE_FASTSEEK
        _file.cltbl = 0;
    #endif
        _file.flag = mode;
        _file.err = 0;
        _file.sect = 0;
        _file.fptr = 0;
        return FR_OK;
        }

    FRESULT Close() { return f_close(&_file); }

    FRESULT Read(void* buff, u32 count, u32& bytesRead)
    {
        return f_read(&_file, buff, count, (UINT*)&bytesRead);
    }

    bool ReadExact(void* buff, u32 count)
    {
        UINT bytesRead = 0;
        return f_read(&_file, buff, count, &bytesRead) == FR_OK && bytesRead == count;
    }

    FRESULT Write(const void* buff, u32 count, u32& bytesWritten)
    {
        return f_write(&_file, buff, count, (UINT*)&bytesWritten);
    }

    FRESULT Seek(FSIZE_t offset) { return f_lseek(&_file, offset); }
    FRESULT Truncate() { return f_truncate(&_file); }
    FRESULT Sync() { return f_sync(&_file); }

    bool IsEof() { return f_eof(&_file); }
    FSIZE_t GetOffset() { return f_tell(&_file); }
    FSIZE_t GetSize() { return f_size(&_file); }
    FRESULT Rewind() { return f_rewind(&_file); }

    FRESULT CreateClusterTable(DWORD* clusterTable, u32 byteSize)
    {
        clusterTable[0] = byteSize / sizeof(DWORD);
        _file.cltbl = clusterTable;
        return f_lseek(&_file, CREATE_LINKMAP);
    }

    File() { }

    ~File()
    {
        Close();
    }
};