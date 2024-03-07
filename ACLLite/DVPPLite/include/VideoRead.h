/**
 * ============================================================================
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#ifndef VIDEO_READ_H
#define VIDEO_READ_H

#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include "acllite_common/Common.h"
#include "acllite_common/ThreadSafeQueue.h"
#include "VdecHelper.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#define RTSP_TRANS_UDP ((uint32_t)0)
#define RTSP_TRANS_TCP ((uint32_t)1)
#define INVALID_CHANNEL_ID (-1)
#define INVALID_STREAM_FORMAT (-1)
#define VIDEO_CHANNEL_MAX  (256)
#define RTSP_TRANSPORT_UDP "udp"
#define RTSP_TRANSPORT_TCP "tcp"

namespace acllite {
typedef int (*FrameProcessCallBack)(void* callback_param, void *frame_data,
                                    int frame_size);

class ChannelIdGenerator {
public:
    ChannelIdGenerator()
    {
        for (int i = 0; i < VIDEO_CHANNEL_MAX; i++) {
            channelId_[i] = INVALID_CHANNEL_ID;
        }
    }
    ~ChannelIdGenerator(){};
 
    int GenerateChannelId(void)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        for (int i = 0; i < VIDEO_CHANNEL_MAX; i++) {
            if (channelId_[i] == INVALID_CHANNEL_ID) {
                channelId_[i] = i;
                return i;
            }
        }

        return INVALID_CHANNEL_ID;
    }

    void ReleaseChannelId(int channelId)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        if ((channelId >= 0) && (channelId < VIDEO_CHANNEL_MAX)) {
            channelId_[channelId] = INVALID_CHANNEL_ID;
        }
    }

private:
    int channelId_[VIDEO_CHANNEL_MAX];
    mutable std::mutex mutex_lock_;
};

class FFmpegDecoder {
public:
    FFmpegDecoder(const std::string& name);
    ~FFmpegDecoder() {}
    void Decode(FrameProcessCallBack callback_func, void *callback_param);
    int GetFrameWidth()
    {
        return frameWidth_;
    }
    int GetFrameHeight()
    {
        return frameHeight_;
    }
    int GetVideoType()
    {
        return videoType_;
    }
    int GetFps()
    {
        return fps_;
    }
    int IsFinished()
    {
        return isFinished_;
    }
    int GetProfile()
    {
        return profile_;
    }
    void SetTransport(const std::string& transportType);
    void StopDecode()
    {
        isStop_ = true;
    }

private:
    int GetVideoIndex(AVFormatContext* av_format_context);
    void GetVideoInfo();
    void InitVideoStreamFilter(const AVBitStreamFilter* &video_filter);
    bool OpenVideo(AVFormatContext*& av_format_context);
    void SetDictForRtsp(AVDictionary* &avdic);
    bool InitVideoParams(int videoIndex,
                         AVFormatContext* av_format_context,
                         AVBSFContext* &bsf_ctx);

private:
    bool isFinished_;
    bool isStop_;
    int frameWidth_;
    int frameHeight_;
    int videoType_;
    int profile_;
    int fps_;
    std::string streamName_;
    std::string rtspTransport_;
};

class VideoRead {
public:
    /**
     * @brief VideoRead constructor
     */
    VideoRead(const std::string& videoName, int32_t deviceId = 0, aclrtContext context = nullptr);

    /**
     * @brief VideoRead destructor
     */
    ~VideoRead();
    bool Read(ImageData& image);
    bool IsOpened();
    bool Set(StreamProperty propId, int value);
    uint32_t Get(StreamProperty propId);
    void Release();
    

private:
    Result Open();
    Result SetAclContext();
    Result InitResource();
    Result InitVdecDecoder();
    Result InitFFmpegDecoder();
    static void FrameDecodeThreadFunction(void* decoderSelf);
    static Result FrameDecodeCallback(void* context, void* frameData,
                                          int frameSize);
    static void DvppVdecCallback(acldvppStreamDesc *input,
                                 acldvppPicDesc *output, void *userdata);
    void ProcessDecodedImage(std::shared_ptr<ImageData> frameData);
    void StartFrameDecoder();
    int GetVdecType();
    void FFmpegDecode()
    {
        ffmpegDecoder_->Decode(&VideoRead::FrameDecodeCallback, (void*) this);
    }
    Result FrameImageEnQueue(std::shared_ptr<ImageData> frameData);
    std::shared_ptr<ImageData> FrameImageOutQueue(bool noWait = false);
    void SleeptoNextFrameTime();
    void SetStatus(DecodeStatus status)
    {
        status_ = status;
    }
    DecodeStatus GetStatus()
    {
        return status_;
    }
    void SetEnd()
    {
        isFrameDecodeEnd_ = true;
    }
    bool GetEnd()
    {
        return isFrameDecodeEnd_;
    }
    bool IsStop()
    {
        return isStop_;
    }
    Result SetRtspTransType(uint32_t transCode);
    void DestroyResource();

private:
    bool isStop_;
    bool isReleased_;
    bool isFrameDecodeEnd_;
    StreamType streamType_;
    DecodeStatus status_;
    int32_t deviceId_;
    aclrtContext context_;
    aclrtRunMode runMode_;
    int channelId_;
    int streamFormat_;
    uint32_t frameId_;
    uint32_t finFrameCnt_;
    int64_t lastDecodeTime_;
    int64_t fpsInterval_;
    std::string streamName_;
    std::thread decodeThread_;
    FFmpegDecoder* ffmpegDecoder_;
    VdecHelper* dvppVdec_;
    ThreadSafeQueue<std::shared_ptr<ImageData>> frameImageQueue_;
    int videoChannelMax_;
};
}  // namespace acllite
#endif /* VIDEO_READ_H_ */
