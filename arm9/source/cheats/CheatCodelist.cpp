/*
    Adapted from TWiLight Menu++
    https://github.com/DS-Homebrew/TWiLightMenu
*/

#include "common.h"
#include <string.h>
#include <stdlib.h>
#include "CheatCodelist.h"

#define CRCPOLY 0xedb88320

static inline u32 Swap32(u32 value)
{
    return (value >> 24)
        | ((value >> 8) & 0x0000FF00)
        | ((value << 8) & 0x00FF0000)
        | (value << 24);
}

static inline u32 ReadU32LE(const u8* data)
{
    return (u32)data[0]
        | ((u32)data[1] << 8)
        | ((u32)data[2] << 16)
        | ((u32)data[3] << 24);
}

// CRC32 calculation based on TWiLightMenu original implementation
u32 CheatCodelist::ComputeCrc32(const u8* p, u32 len)
{
    u32 crc = 0xFFFFFFFF;
    while (len--)
    {
        crc ^= *p++;
        for (int ii = 0; ii < 8; ++ii)
            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY : 0);
    }
    return crc;
}

bool CheatCodelist::ReadRomData(const FastFileRef& romFastFileRef, u32& outGameCode, u32& outCrc32)
{
    File file;
    file.Open(romFastFileRef, FA_READ);

    if (file.GetSize() < 512)
    {
        return false;
    }

    u8 header[512];
    if (!file.ReadExact(header, sizeof(header)))
    {
        return false;
    }

    outCrc32 = ComputeCrc32(header, sizeof(header));
    memcpy(&outGameCode, header + 0x0C, sizeof(outGameCode));

    return true;
}

bool CheatCodelist::SearchCheatData(File& datFile, u32 gamecode, u32 crc32, long& pos, u32& size)
{
    pos = 0;
    size = 0;

    if (datFile.Seek(0) != FR_OK)
        return false;

    auto readIndex = [&datFile](DatIndex& outIdx) -> bool
    {
        u8 buf[16];
        u32 bytesRead = 0;
        if (datFile.Read(buf, sizeof(buf), bytesRead) != FR_OK || bytesRead != sizeof(buf))
            return false;

        outIdx._gameCode = ReadU32LE(buf);
        outIdx._crc32 = ReadU32LE(buf + 4);
        outIdx._offset = ReadU32LE(buf + 8) | (static_cast<u64>(ReadU32LE(buf + 12)) << 32);
        return true;
    };

    DatIndex idx, nidx;

    FSIZE_t fileSize = datFile.GetSize();

    if (datFile.Seek(0x100) != FR_OK)
        return false;

    if (!readIndex(nidx))
        return false;

    bool done = false;
    while (!done)
    {
        memcpy(&idx, &nidx, sizeof(idx));
        
        bool nextReadSuccess = readIndex(nidx);
        if (!nextReadSuccess)
        {
            memset(&nidx, 0, sizeof(nidx));
        }

        if (gamecode == idx._gameCode && (crc32 == idx._crc32 || Swap32(crc32) == idx._crc32))
        {
            size = (u32)((nidx._offset ? nidx._offset : fileSize) - idx._offset);
            pos = (long)idx._offset;
            done = true;
        }
        
        if (!nidx._offset && !nextReadSuccess) 
            done = true;
        else if (!nidx._offset) 
            done = true;
    }
    return (pos != 0 && size != 0);
}

bool CheatCodelist::ParseCheatData(File& datFile, u32 gamecode, u32 crc32)
{
    _items.Clear();

    long dataPos;
    u32 dataSize;
    if (!SearchCheatData(datFile, gamecode, crc32, dataPos, dataSize)) {
        return false;
    }

    if (datFile.Seek(dataPos) != FR_OK) {
        return false;
    }

    char* buffer = (char*)malloc(dataSize);
    if (!buffer) {
        return false;
    }

    u32 bytesRead;
    if (datFile.Read(buffer, dataSize, bytesRead) != FR_OK || bytesRead != dataSize) {
        free(buffer);
        return false;
    }

    // Binary block layout follows TWiLightMenu usrcheat.dat format
    char* gameTitle = buffer;
    u32* ccode = (u32*)(((u32)gameTitle + strlen(gameTitle) + 4) & ~3);
    u32 cheatCount = *ccode;
    cheatCount &= 0x0FFFFFFF;
    ccode += 9;

    u32 cc = 0;
    while (cc < cheatCount) {
        u32 folderCount = 1;
        u32 flagItem = 0;

        // Flag handling (EFolder, EOne, ESelected) based on TWiLightMenu
        if ((*ccode >> 28) & 1) {
            flagItem |= CheatItem::EInFolder;
            if ((*ccode >> 24) == 0x11)
                flagItem |= CheatItem::EOne;

            folderCount = *ccode & 0x00FFFFFF;
            char* folderName = (char*)((u32)ccode + 4);
            char* folderNote = (char*)((u32)folderName + strlen(folderName) + 1);

            _items.Push(CheatItem(folderName, folderNote, CheatItem::EFolder));
            cc++;
            // 4-byte alignment using &~3 as in TWiLightMenu
            ccode = (u32*)(((u32)folderName + strlen(folderName) + 1 + strlen(folderNote) + 1 + 3) & ~3);
        }

        u32 selectValue = CheatItem::ESelected;
        for (u32 ii = 0; ii < folderCount; ++ii) {
            char* cheatName = (char*)((u32)ccode + 4);
            char* cheatNote = (char*)((u32)cheatName + strlen(cheatName) + 1);
            // Cheat data length reading derived from TWiLightMenu format
            u32* cheatData = (u32*)(((u32)cheatNote + strlen(cheatNote) + 1 + 3) & ~3); 

            u32 cheatDataLen = *cheatData++;

            if (cheatDataLen) {
                u32 flags = flagItem | ((*ccode & 0xFF000000) ? selectValue : 0);
                long cheatOffset = dataPos + (long)(((char*)ccode + 3) - buffer);

                CheatItem item(cheatName, cheatNote, flags, cheatOffset);
                item.SetCheatCodes(cheatData, cheatDataLen);
                _items.Push(static_cast<CheatItem&&>(item));

                if ((*ccode & 0xFF000000) && (flagItem & CheatItem::EOne))
                    selectValue = 0;
            }
            cc++;
            ccode = (u32*)((u32)ccode + (((*ccode & 0x00FFFFFF) + 1) * 4));
        }
    }

    free(buffer);
    BuildVisibleList();
    return true;
}

CheatParseResult CheatCodelist::Parse(const FastFileRef& romFastFileRef)
{
    u32 gamecodeVal = 0;
    u32 crc32Val = 0;

    ReadRomData(romFastFileRef, gamecodeVal, crc32Val);

    memcpy(_gameCode, &gamecodeVal, 4);
    _gameCode[4] = 0;

    auto datFile = std::make_unique<File>();
    if (datFile->Open("/_pico/extras/usrcheat.dat", FA_READ) != FR_OK) {
        return CheatParseResult::DatFileNotFound;
    }

    bool result = ParseCheatData(*datFile, gamecodeVal, crc32Val);
    return result ? CheatParseResult::Success : CheatParseResult::NoCheatsFound;
}

void CheatCodelist::BuildVisibleList()
{
    _isEnabledListMode = false;
    BuildVisibleListForFolder(-1);
}

void CheatCodelist::BuildVisibleListEnabledOnly()
{
    _isEnabledListMode = true;
    _visibleIndices.Clear();
    for (int i = 0; i < _items.size(); ++i)
    {
        if ( !(_items[i].flags & CheatItem::EFolder) && (_items[i].flags & CheatItem::ESelected) )
        {
            _visibleIndices.Push(i);
        }
    }
}

void CheatCodelist::BuildVisibleListForFolder(int folderIndex)
{
    _isEnabledListMode = false;
    _visibleIndices.Clear();

    if (folderIndex == -1)
    {
        for (int i = 0; i < _items.size(); ++i)
        {
            u32 flags = _items[i].flags;
            if ((flags & CheatItem::EFolder) || !(flags & CheatItem::EInFolder))
            {
                _visibleIndices.Push(i);
            }
        }
    }
    else
    {
       
        for (int i = folderIndex + 1; i < _items.size(); ++i)
        {

            if (_items[i].flags & CheatItem::EFolder)
            {
               break;
            }
            
            if (!(_items[i].flags & CheatItem::EInFolder))
            {
               break;
            }
            
            _visibleIndices.Push(i);
        }
    }
}

bool CheatCodelist::UpdateUsrCheatDat(const char* usrCheatPath)
{
    File file;
    if (file.Open(usrCheatPath, FA_READ | FA_WRITE) != FR_OK)
        return false;

    u32 bytesWritten;
    u8 flagByte;
    
    for (int i = 0; i < _items.size(); i++)
    {
        long offset = _items[i].dataOffset;
        if (offset == 0) continue; 

        if (file.Seek(offset) != FR_OK) continue;
        
        u32 br;
        if (file.Read(&flagByte, 1, br) != FR_OK || br != 1) continue;
        
        if (_items[i].flags & CheatItem::ESelected)
            flagByte |= 0x01; 
        flagByte = (_items[i].flags & CheatItem::ESelected) ? 0x01 : 0x00;
        
        file.Seek(offset);
        file.Write(&flagByte, 1, bytesWritten);
    }
    
    file.Sync();
    return true;
}

u32 CheatCodelist::GetSelectedCheatCodeCount() const
{
    u32 total = 0;
    for (int i = 0; i < _items.size(); i++)
    {
        if (_items[i].flags & CheatItem::ESelected)
            total += _items[i].cheatCodeCount;
    }
    return total;
}

u32 CheatCodelist::CopySelectedCheats(u32* outBuf) const
{
    u32 pos = 0;
    for (int i = 0; i < _items.size(); i++)
    {
        if (_items[i].flags & CheatItem::ESelected)
        {
            memcpy(outBuf + pos, _items[i].cheatCodes, _items[i].cheatCodeCount * sizeof(u32));
            pos += _items[i].cheatCodeCount;
        }
    }
    return pos;
}
