#pragma once
#include "fat/ff.h"
#include "fat/FastFileRef.h"
#include "FileType/FileType.h"

class InternalFileInfo;

class FileInfo
{
public:
    FileInfo() { }
    FileInfo(const FileInfo& fileInfo);
    FileInfo(const TCHAR* fileName, const FileType* type, const FastFileRef& fastFileRef, u8 attributes,
        const TCHAR* fullPath = nullptr);

    FileInfo &operator=(FileInfo&& rhs)
    {
        if (this != &rhs)
        {
            _name = std::move(rhs._name);
            _type = rhs._type;
            _fastFileRef = rhs._fastFileRef;
            _attributes = rhs._attributes;
            _fullPath = std::move(rhs._fullPath);
        }

        return *this;
    }

    const TCHAR* GetFileName() const { return _name.get(); }
    const FileType* GetFileType() const { return _type; }
    u32 GetFileSize() const { return _fastFileRef.GetFileSize(); }

    /// @brief Gets the full path of this file, if it was constructed with one
    ///        (currently only true for synthetic entries in the favorites list).
    ///        Returns nullptr otherwise.
    const TCHAR* GetFullPath() const { return _fullPath.get(); }

    InternalFileInfo* CreateInternalFileInfo() const
    {
        return _type->CreateInternalFileInfo(_fastFileRef);
    }

    const FastFileRef& GetFastFileRef() const { return _fastFileRef; }

    bool IsReadOnly() const { return _attributes & AM_RDO; }
    bool IsHidden() const { return _attributes & AM_HID; }
    bool IsSystem() const { return _attributes & AM_SYS; }

private:
    std::unique_ptr<TCHAR[]> _name;
    const FileType* _type;
    FastFileRef _fastFileRef;
    u8 _attributes;
    std::unique_ptr<TCHAR[]> _fullPath;
};
