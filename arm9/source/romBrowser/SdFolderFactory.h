#pragma once
#include <memory>
#include "SdFolder.h"
#include "FileType/IFileTypeProvider.h"

class SdFolderFactory
{
public:
    SdFolderFactory(const IFileTypeProvider* fileTypeProvider,
        const char* const* hidePatterns = nullptr, u32 hidePatternCount = 0)
        : _fileTypeProvider(fileTypeProvider)
        , _hidePatterns(hidePatterns)
        , _hidePatternCount(hidePatternCount) { }

    std::unique_ptr<SdFolder> CreateFromPath(const char* path) const;

private:
    const IFileTypeProvider* _fileTypeProvider;
    const char* const* _hidePatterns;
    u32 _hidePatternCount;
};