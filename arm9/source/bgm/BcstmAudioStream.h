#pragma once
#include "common.h"
#include <memory>
#include "fat/File.h"
#include "IAudioStream.h"
#include "bcstm.h"

struct DspAdpcmContext
{
    const s16* coefTable;
    const u8* data;
    s16 frameData[14];
    u8 frameIdx;

    void SetData(const u8* data)
    {
        this->data = data;
        frameIdx = 0;
    }
};

class alignas(32) BcstmAudioStream : public IAudioStream
{
public:
    /// @brief Opens the given file. Call this function before
    ///        using ReadSamples.
    /// @param filePath The path of the file to open.
    /// @return True if the file was successfully opened, or false otherwise.
    bool Open(const TCHAR* filePath);

    /// @brief Opens the given file. Call this function before
    ///        using ReadSamples.
    /// @param filePath The path of the file to open.
    /// @return True if the file was successfully opened, or false otherwise.
    bool Open(const FastFileRef& fastFileRef);

    void Close() override;
    void ReadSamples(s16* left, s16* right, u32 count) override;

    u32 GetSampleRate() const override
    {
        return _streamInfo.sampleRate;
    }

private:
    File _audioFile alignas(32);
    DWORD _clusterTable[1024] alignas(32);
    bcstm_info_stream_t _streamInfo;
    bcstm_dspadpcm_t _dspAdpcmInfo[2];
    u32 _dataOffset;
    std::unique_ptr<u8[]> _adpcmBlocks[2];
    u32 _channels;
    u32 _blockNumber;
    u32 _sampleNumberInBlock;
    u32 _loopBlockNumber;
    u32 _loopEndBlockNumber;
    u32 _loopBlockStartSample;
    u32 _loopEndBlockEndSample;
    DspAdpcmContext _dspAdpcmContexts[2];

    bool TryLoadBcstm();
    u32 GetTotalSamplesInCurrentBlock();
    void FetchBlock();
    void DecodeAdpcmSamples(DspAdpcmContext& context, s16* dst, u32 count);
    void DecodeLoopAdpcmFrame(DspAdpcmContext& context, u32 scaleCoef, s16 last1, s16 last2, u32 sampleOffset);
};
