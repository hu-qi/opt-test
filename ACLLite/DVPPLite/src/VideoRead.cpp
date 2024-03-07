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
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <fstream>
#include <memory>
#include <thread>
#include <cstring>
#include <iostream>
#include "acllite_common/Common.h"
#include "VideoRead.h"

using namespace std;
#define DEVICE_MAX  4

namespace acllite {
    const int64_t kUsec = 1000000;
    const uint32_t kDecodeFrameQueueSize = 256;
    const int kDecodeQueueOpWait = 10000; // decode wait 10ms/frame
    const int kFrameEnQueueRetryTimes = 1000; // max wait time for the frame to enter in queue
    const int kQueueOpRetryTimes = 1000;
    const int kOutputJamWait = 10000;
    const int kInvalidTpye = -1;
    const int kWaitDecodeFinishInterval = 1000;
    const int kDefaultFps = 1;
    const int kReadSlow = 5;
    const uint32_t kVideoChannelMax310 = 32;
    const uint32_t kVideoChannelMax310P = 256;
    const uint32_t kVideoChannelMax310B = 128;

    ChannelIdGenerator channelIdGenerator[DEVICE_MAX] = {};

    const int kNoFlag = 0; // no flag
    const int kInvalidVideoIndex = -1; // invalid video index
    const string kRtspTransport = "rtsp_transport"; // rtsp transport
    const string kUdp = "udp"; // video format udp
    const string kTcp = "tcp";
    const string kBufferSize = "buffer_size"; // buffer size string
    const string kMaxBufferSize = "10485760"; // maximum buffer size:10MB
    const string kMaxDelayStr = "max_delay"; // maximum delay string
    const string kMaxDelayValue = "100000000"; // maximum delay time:100s
    const string kTimeoutStr = "stimeout"; // timeout string
    const string kTimeoutValue = "5000000"; // timeout:5s
    const string kPktSize = "pkt_size"; // ffmpeg pakect size string
    const string kPktSizeValue = "10485760"; // ffmpeg packet size value:10MB
    const string kReorderQueueSize = "reorder_queue_size"; // reorder queue size
    const string kReorderQueueSizeValue = "0"; // reorder queue size value
    const int kErrorBufferSize = 1024; // buffer size for error info
    const uint32_t kDefaultStreamFps = 5;
    const uint32_t kOneSecUs = 1000 * 1000;
}

namespace acllite {
FFmpegDecoder::FFmpegDecoder(const std::string& streamName):streamName_(streamName)
{
    rtspTransport_.assign(kTcp.c_str());
    isFinished_ = false;
    isStop_ = false;
    GetVideoInfo();
}

void FFmpegDecoder::SetTransport(const std::string& transportType)
{
    rtspTransport_.assign(transportType.c_str());
};

int FFmpegDecoder::GetVideoIndex(AVFormatContext* avFormatContext)
{
    if (avFormatContext == nullptr) { // verify input pointer
        return kInvalidVideoIndex;
    }

    // get video index in streams
    for (uint32_t i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type
            == AVMEDIA_TYPE_VIDEO) { // check is media type is video
            return i;
        }
    }

    return kInvalidVideoIndex;
}

void FFmpegDecoder::InitVideoStreamFilter(const AVBitStreamFilter*& videoFilter)
{
    if (videoType_ == AV_CODEC_ID_H264) { // check video type is h264
        videoFilter = av_bsf_get_by_name("h264_mp4toannexb");
    } else { // the video type is h265
        videoFilter = av_bsf_get_by_name("hevc_mp4toannexb");
    }
}

void FFmpegDecoder::SetDictForRtsp(AVDictionary*& avdic)
{
    LOG_PRINT("[INFO] Set parameters for %s", streamName_.c_str());

    av_dict_set(&avdic, kRtspTransport.c_str(), rtspTransport_.c_str(), kNoFlag);
    av_dict_set(&avdic, kBufferSize.c_str(), kMaxBufferSize.c_str(), kNoFlag);
    av_dict_set(&avdic, kMaxDelayStr.c_str(), kMaxDelayValue.c_str(), kNoFlag);
    av_dict_set(&avdic, kTimeoutStr.c_str(), kTimeoutValue.c_str(), kNoFlag);
    av_dict_set(&avdic, kReorderQueueSize.c_str(),
                kReorderQueueSizeValue.c_str(), kNoFlag);
    av_dict_set(&avdic, kPktSize.c_str(), kPktSizeValue.c_str(), kNoFlag);
    LOG_PRINT("[INFO] Set parameters for %s end", streamName_.c_str());
}

bool FFmpegDecoder::OpenVideo(AVFormatContext*& avFormatContext)
{
    bool ret = true;
    AVDictionary* avdic = nullptr;

    av_log_set_level(AV_LOG_DEBUG);

    LOG_PRINT("[INFO] Open video %s ...", streamName_.c_str());
    SetDictForRtsp(avdic);
    int openRet = avformat_open_input(&avFormatContext,
                                      streamName_.c_str(), nullptr,
                                      &avdic);
    if (openRet < 0) { // check open video result
        char buf_error[kErrorBufferSize];
        av_strerror(openRet, buf_error, kErrorBufferSize);

        LOG_PRINT("[ERROR] Could not open video:%s, return :%d, error info:%s",
                          streamName_.c_str(), openRet, buf_error);
        ret = false;
    }

    if (avdic != nullptr) { // free AVDictionary
        av_dict_free(&avdic);
    }

    return ret;
}

bool FFmpegDecoder::InitVideoParams(int videoIndex,
                                    AVFormatContext* avFormatContext,
                                    AVBSFContext*& bsfCtx)
{
    const AVBitStreamFilter* videoFilter = nullptr;
    InitVideoStreamFilter(videoFilter);
    if (videoFilter == nullptr) { // check video fileter is nullptr
        LOG_PRINT("[ERROR] Unkonw bitstream filter, videoFilter is nullptr!");
        return false;
    }

    // checke alloc bsf context result
    if (av_bsf_alloc(videoFilter, &bsfCtx) < 0) {
        LOG_PRINT("[ERROR] Fail to call av_bsf_alloc!");
        return false;
    }

    // check copy parameters result
    if (avcodec_parameters_copy(bsfCtx->par_in,
        avFormatContext->streams[videoIndex]->codecpar) < 0) {
        LOG_PRINT("[ERROR] Fail to call avcodec_parameters_copy!");
        return false;
    }

    bsfCtx->time_base_in = avFormatContext->streams[videoIndex]->time_base;

    // check initialize bsf contextreult
    if (av_bsf_init(bsfCtx) < 0) {
        LOG_PRINT("[ERROR] Fail to call av_bsf_init!");
        return false;
    }

    return true;
}

void FFmpegDecoder::Decode(FrameProcessCallBack callback,
                           void *callbackParam)
{
    LOG_PRINT("[INFO] Start ffmpeg decode video %s ...", streamName_.c_str());
    avformat_network_init(); // init network

    AVFormatContext* avFormatContext = avformat_alloc_context();

    // check open video result
    if (!OpenVideo(avFormatContext)) {
        return;
    }

    int videoIndex = GetVideoIndex(avFormatContext);
    if (videoIndex == kInvalidVideoIndex) { // check video index is valid
        LOG_PRINT("[ERROR] Rtsp %s index is -1", streamName_.c_str());
        return;
    }

    AVBSFContext* bsfCtx = nullptr;
    // check initialize video parameters result
    if (!InitVideoParams(videoIndex, avFormatContext, bsfCtx)) {
        return;
    }

    LOG_PRINT("[INFO] Start decode frame of video %s ...", streamName_.c_str());

    AVPacket avPacket;
    int processOk = true;
    // loop to get every frame from video stream
    while ((av_read_frame(avFormatContext, &avPacket) == 0) && processOk && !isStop_) {
        if (avPacket.stream_index == videoIndex) { // check current stream is video
          // send video packet to ffmpeg
            if (av_bsf_send_packet(bsfCtx, &avPacket)) {
                LOG_PRINT("[ERROR] Fail to call av_bsf_send_packet, channel id:%s",
                    streamName_.c_str());
            }

            // receive single frame from ffmpeg
            while ((av_bsf_receive_packet(bsfCtx, &avPacket) == 0) && !isStop_) {
                int ret = callback(callbackParam, avPacket.data, avPacket.size);
                if (ret != 0) {
                    processOk = false;
                    break;
                }
            }
        }
        av_packet_unref(&avPacket);
    }

    av_bsf_free(&bsfCtx); // free AVBSFContext pointer
    avformat_close_input(&avFormatContext); // close input video

    isFinished_ = true;
    LOG_PRINT("[INFO] Ffmpeg decoder %s finished", streamName_.c_str());
}

void FFmpegDecoder::GetVideoInfo()
{
    avformat_network_init(); // init network
    AVFormatContext* avFormatContext = avformat_alloc_context();
    bool ret = OpenVideo(avFormatContext);
    if (ret == false) {
        LOG_PRINT("[ERROR] Open %s failed", streamName_.c_str());
        return;
    }

    if (avformat_find_stream_info(avFormatContext, NULL)<0) {
        LOG_PRINT("[ERROR] Get stream info of %s failed", streamName_.c_str());
        return;
    }

    int videoIndex = GetVideoIndex(avFormatContext);
    if (videoIndex == kInvalidVideoIndex) { // check video index is valid
        LOG_PRINT("[ERROR] Video index is %d, current media stream has no "
                          "video info:%s",
                          kInvalidVideoIndex, streamName_.c_str());
        avformat_close_input(&avFormatContext);
        return;
    }

    AVStream* inStream = avFormatContext->streams[videoIndex];

    frameWidth_ = inStream->codecpar->width;
    frameHeight_ = inStream->codecpar->height;
    if (inStream->avg_frame_rate.den) {
        fps_ = inStream->avg_frame_rate.num / inStream->avg_frame_rate.den;
    } else {
        fps_ = kDefaultStreamFps;
    }

    videoType_ = inStream->codecpar->codec_id;
    profile_ = inStream->codecpar->profile;

    avformat_close_input(&avFormatContext);

    LOG_PRINT("[INFO] Video %s, type %d, profile %d, width:%d, height:%d, fps:%d",
                     streamName_.c_str(), videoType_, profile_, frameWidth_, frameHeight_, fps_);
    return;
}

VideoRead::VideoRead(const std::string& videoName, int32_t deviceId, aclrtContext context)
    :isStop_(false), isReleased_(false), isFrameDecodeEnd_(false),
    streamType_(STREAM_VIDEO), status_(DECODE_UNINIT),
    deviceId_(deviceId), context_(context),
    channelId_(INVALID_CHANNEL_ID), streamFormat_(H264_MAIN_LEVEL),
    frameId_(0), finFrameCnt_(0), lastDecodeTime_(0),
    fpsInterval_(0), streamName_(videoName), ffmpegDecoder_(nullptr),
    dvppVdec_(nullptr), frameImageQueue_(kDecodeFrameQueueSize)
{
    const string kRegexRtsp = "^rtsp://.*";
    regex regexRtspAddress(kRegexRtsp.c_str());
    if(regex_match(streamName_, regexRtspAddress)) {
        streamType_ = STREAM_RTSP;
    }
    Open();

}

VideoRead::~VideoRead()
{
    DestroyResource();
}

void VideoRead::DestroyResource()
{
    if (isReleased_) return;
    // 1. stop ffmpeg
    isStop_ = true;

    // 2. delete ffmpeg decoder
    if(ffmpegDecoder_ != nullptr){
        ffmpegDecoder_->StopDecode();
        while ((status_ >= DECODE_START) && (status_ < DECODE_FFMPEG_FINISHED)) {
            usleep(kWaitDecodeFinishInterval);
        }
        delete ffmpegDecoder_;
        ffmpegDecoder_ = nullptr;
    }
 
    // 3. release dvpp vdec
    if(dvppVdec_ != nullptr) {
        while (!isFrameDecodeEnd_) {
            usleep(kWaitDecodeFinishInterval);
        }
        delete dvppVdec_;
        dvppVdec_ = nullptr;
    }
    // 4. release image memory in decode output queue
    do {
        shared_ptr<ImageData> frame = FrameImageOutQueue(true);
        if (frame == nullptr) {
            break;
        }

        if (frame->data != nullptr) {
            acldvppFree(frame->data.get());
            frame->data = nullptr;
        }
    } while (1);
    // 5. release channel id
    channelIdGenerator[deviceId_].ReleaseChannelId(channelId_);

    isReleased_ = true;
}

Result VideoRead::InitResource()
{
    aclError aclRet;
    // use current thread context default
    if (context_ == nullptr) {
        aclRet = aclrtGetCurrentContext(&context_);
        if ((aclRet != ACL_SUCCESS) || (context_ == nullptr)) {
            LOG_PRINT("[ERROR] Get current acl context error:%d", aclRet);
            return RES_ERROR;
        }
    }
    // Get current run mode
    aclRet = aclrtGetRunMode(&runMode_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] acl get run mode failed");
        return RES_ERROR;
    }

    return RES_OK;
}

Result VideoRead::InitVdecDecoder()
{
    string socVersion = aclrtGetSocName();
    size_t pos310P = socVersion.find("Ascend310P");
    size_t pos310B = socVersion.find("Ascend310B");
    if (pos310P != socVersion.npos) {
        videoChannelMax_ = kVideoChannelMax310P;
    } else if (pos310B != socVersion.npos) {
        videoChannelMax_ = kVideoChannelMax310B;
    } else {
        videoChannelMax_ = kVideoChannelMax310;
    }

    // Generate a unique channel id for video decoder
    channelId_ = channelIdGenerator[deviceId_].GenerateChannelId();
    if (channelId_ == INVALID_CHANNEL_ID || channelId_ >= videoChannelMax_) {
        LOG_PRINT("[ERROR] Decoder number excessive %d", videoChannelMax_);
        return RES_ERROR;
    }

    // Create dvpp vdec to decode h26x data
    dvppVdec_ = new VdecHelper(channelId_, ffmpegDecoder_->GetFrameWidth(),
                               ffmpegDecoder_->GetFrameHeight(),
                               streamFormat_, VideoRead::DvppVdecCallback);
    Result ret = dvppVdec_->Init();
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Dvpp vdec init failed");
    }

    return ret;
}

Result VideoRead::InitFFmpegDecoder()
{
    // Create ffmpeg decoder to parse video stream to h26x frame data
    ffmpegDecoder_ = new FFmpegDecoder(streamName_);
    if (kInvalidTpye == GetVdecType()) {
        this->SetStatus(DECODE_ERROR);
        if (ffmpegDecoder_ != nullptr) {
            delete ffmpegDecoder_;
            ffmpegDecoder_ = nullptr;
        }
        LOG_PRINT("[ERROR] Video %s type is invalid", streamName_.c_str());
        return RES_ERROR;
    }

    // Get video fps, if no fps, use 1 as default
    int fps =  ffmpegDecoder_->GetFps();
    if (fps == 0) {
        fps = kDefaultFps;
        LOG_PRINT("[INFO] Video %s fps is 0, change to %d",
                         streamName_.c_str(), fps);
    }
    // Cal the frame interval time(us)
    fpsInterval_ = kUsec / fps;

    return RES_OK;
}

Result VideoRead::Open()
{
    // Open video stream, if open failed before, return error directly
    if (status_ == DECODE_ERROR)
        return RES_ERROR;
    // If open ok already
    if (status_ != DECODE_UNINIT)
        return RES_OK;
    // Init acl resource
    Result ret = InitResource();
    if (ret != RES_OK) {
        this->SetStatus(DECODE_ERROR);
        LOG_PRINT("[ERROR] Open %s failed for init resource error: %d",
                          streamName_.c_str(), ret);
        return ret;
    }
    // Init ffmpeg decoder
    ret = InitFFmpegDecoder();
    if (ret != RES_OK) {
        this->SetStatus(DECODE_ERROR);
        LOG_PRINT("[ERROR] Open %s failed for init ffmpeg error: %d",
                          streamName_.c_str(), ret);
        return ret;
    }
    // Init dvpp vdec decoder
    ret = InitVdecDecoder();
    if (ret != RES_OK) {
        this->SetStatus(DECODE_ERROR);
        LOG_PRINT("[ERROR] Open %s failed for init vdec error: %d",
                          streamName_.c_str(), ret);
        return ret;
    }
    // Set init ok
    this->SetStatus(DECODE_READY);
    LOG_PRINT("[INFO] Video %s decode init ok", streamName_.c_str());
    return RES_OK;
}

int VideoRead::GetVdecType()
{
    // VDEC only support H265 main level�?64 baseline level，main level，high level
    int type = ffmpegDecoder_->GetVideoType();
    int profile = ffmpegDecoder_->GetProfile();
    if (type == AV_CODEC_ID_HEVC) {
        streamFormat_ = H265_MAIN_LEVEL;
    } else if (type == AV_CODEC_ID_H264) {
        switch (profile) {
            case FF_PROFILE_H264_BASELINE:
                streamFormat_ = H264_BASELINE_LEVEL;
                break;
            case FF_PROFILE_H264_MAIN:
                streamFormat_ = H264_MAIN_LEVEL;
                break;
            case FF_PROFILE_H264_HIGH:
            case FF_PROFILE_H264_HIGH_10:
            case FF_PROFILE_H264_HIGH_10_INTRA:
            case FF_PROFILE_H264_MULTIVIEW_HIGH:
            case FF_PROFILE_H264_HIGH_422:
            case FF_PROFILE_H264_HIGH_422_INTRA:
            case FF_PROFILE_H264_STEREO_HIGH:
            case FF_PROFILE_H264_HIGH_444:
            case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
            case FF_PROFILE_H264_HIGH_444_INTRA:
                streamFormat_ = H264_HIGH_LEVEL;
                break;
            default:
                LOG_PRINT("[INFO] Not support h264 profile %d, use as mp", profile);
                streamFormat_ = H264_MAIN_LEVEL;
                break;
        }
    } else {
        streamFormat_ = kInvalidTpye;
        LOG_PRINT("[ERROR] Not support stream, type %d,  profile %d", type, profile);
    }

    return streamFormat_;
}

// dvpp vdec callback
void VideoRead::DvppVdecCallback(acldvppStreamDesc *input,
                                    acldvppPicDesc *output, void *userData)
{
    VideoRead* decoder = (VideoRead*)userData;
    if (decoder->GetEnd()) {
        return;
    }
    // Get decoded image parameters
    shared_ptr<ImageData> image = make_shared<ImageData>();
    image->format = acldvppGetPicDescFormat(output);
    image->width = acldvppGetPicDescWidth(output);
    image->height = acldvppGetPicDescHeight(output);
    image->alignWidth = acldvppGetPicDescWidthStride(output);
    image->alignHeight = acldvppGetPicDescHeightStride(output);
    image->size = acldvppGetPicDescSize(output);

    void* vdecOutBufferDev = acldvppGetPicDescData(output);
    image->data = SHARED_PTR_DVPP_BUF(vdecOutBufferDev);

    // Put the decoded image to queue for read
    decoder->ProcessDecodedImage(image);
    // Release resouce
    aclError ret = acldvppDestroyPicDesc(output);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] fail to destroy pic desc, error %d", ret);
    }

    if (input != nullptr) {
        void* inputBuf = acldvppGetStreamDescData(input);
        if (inputBuf != nullptr) {
            acldvppFree(inputBuf);
        }
        aclError ret = acldvppDestroyStreamDesc(input);
        if (ret != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] fail to destroy input stream desc");
        }
    }
}

void VideoRead::ProcessDecodedImage(shared_ptr<ImageData> frameData)
{
    finFrameCnt_++;
    if (YUV420SP_SIZE(frameData->alignWidth, frameData->alignHeight) != frameData->size) {
        LOG_PRINT("[ERROR] Invalid decoded frame parameter, "
                          "width %d, height %d, size %d, buffer %p\n",
                          frameData->width, frameData->height,
                          frameData->size, frameData->data.get());
        return;
    }

    FrameImageEnQueue(frameData);

    if ((status_ == DECODE_FFMPEG_FINISHED) && (finFrameCnt_ >= frameId_)) {
        LOG_PRINT("[INFO] Last frame decoded by dvpp, change status to %d",
                         DECODE_DVPP_FINISHED);
        this->SetStatus(DECODE_DVPP_FINISHED);
    }
}

Result VideoRead::FrameImageEnQueue(shared_ptr<ImageData> frameData)
{
    for (int count = 0; count < kFrameEnQueueRetryTimes; count++) {
        if (frameImageQueue_.Push(frameData))
            return RES_OK;
        usleep(kDecodeQueueOpWait);
    }
    LOG_PRINT("[ERROR] Video %s lost decoded image for queue full",
                      streamName_.c_str());

    return RES_ERROR;
}

// start decoder
void VideoRead::StartFrameDecoder()
{
    if (status_ == DECODE_READY) {
        decodeThread_ = thread(FrameDecodeThreadFunction, (void*)this);
        decodeThread_.detach();
        status_ = DECODE_START;
    }
}

// ffmpeg decoder entry
void VideoRead::FrameDecodeThreadFunction(void* decoderSelf)
{
    VideoRead* thisPtr = (VideoRead*)decoderSelf;

    aclError aclRet = thisPtr->SetAclContext();
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Set frame decoder context failed, errorno:%d", aclRet);
        return;
    }
    // start decode until complete
    thisPtr->FFmpegDecode();
    if (thisPtr->IsStop()) {
        thisPtr->SetEnd();
        thisPtr->SetStatus(DECODE_FINISHED);
        return;
    }
    thisPtr->SetStatus(DECODE_FFMPEG_FINISHED);
    // when ffmpeg decode finish, send eos to vdec
    shared_ptr<FrameData> videoFrame = make_shared<FrameData>();
    videoFrame->isFinished = true;
    videoFrame->data = nullptr;
    videoFrame->size = 0;
    thisPtr->dvppVdec_->Process(videoFrame, decoderSelf);
    while ((thisPtr->GetStatus() != DECODE_DVPP_FINISHED)) {
        usleep(kWaitDecodeFinishInterval);
    }
    thisPtr->SetEnd();
}

// callback of ffmpeg decode frame
Result VideoRead::FrameDecodeCallback(void* decoder, void* frameData, int frameSize)
{
    if ((frameData == NULL) || (frameSize == 0)) {
        LOG_PRINT("[ERROR] Frame data is null");
        return RES_ERROR;
    }

    // copy data to dvpp memory
    VideoRead* videoDecoder = (VideoRead*)decoder;
    void* buffer = nullptr;
    aclError aclRet = acldvppMalloc(&buffer, frameSize);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] acldvppMalloc failed. ERROR: %d\n", aclRet);
        return RES_ERROR;
    }
    aclRet = aclrtMemcpy(buffer, frameSize, frameData, frameSize, ACL_MEMCPY_HOST_TO_DEVICE);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d\n", aclRet);
        acldvppFree(frameData);
        return RES_ERROR;
    }

    shared_ptr<FrameData> videoFrame = make_shared<FrameData>();
    videoDecoder->frameId_++;
    videoFrame->frameId = videoDecoder->frameId_;
    videoFrame->data = buffer;
    videoFrame->size = frameSize;
    // decode data by dvpp vdec
    Result ret = videoDecoder->dvppVdec_->Process(videoFrame, decoder);
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Dvpp vdec process %dth frame failed, error:%d",
                          videoDecoder->frameId_, ret);
        return ret;
    }

    // wait next frame by fps
    videoDecoder->SleeptoNextFrameTime();
    return RES_OK;
}

void VideoRead::SleeptoNextFrameTime()
{
    while (frameImageQueue_.Size() >  kReadSlow) {
        if (isStop_) {
            return;
        }
        usleep(kOutputJamWait);
    }

    if (streamType_ == STREAM_RTSP) {
        usleep(0);
        return;
    }

    // get current time
    timeval tv;
    gettimeofday(&tv, 0);
    int64_t now = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;

    if (lastDecodeTime_ == 0) {
        lastDecodeTime_ = now;
        return;
    }
    // calculate interval
    int64_t lastInterval = (now - lastDecodeTime_);
    int64_t sleepTime = (lastInterval < fpsInterval_)?(fpsInterval_-lastInterval):0;
    // consume rest time
    usleep(sleepTime);
    // record start time of next frame
    gettimeofday(&tv, 0);
    lastDecodeTime_ = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;

    return;
}

// check decoder status
bool VideoRead::IsOpened()
{
    LOG_PRINT("[INFO] Video %s decode status %d", streamName_.c_str(), status_);
    return (status_ == DECODE_READY) || (status_ == DECODE_START);
}

// read decoded frame
bool VideoRead::Read(ImageData& image)
{
    // return nullptr,if decode fail/finish
    if (status_ == DECODE_ERROR) {
        LOG_PRINT("[ERROR] Read failed for decode %s failed",
                          streamName_.c_str());
        return false;
    }

    if (status_ == DECODE_FINISHED) {
        LOG_PRINT("[INFO] No frame to read for decode %s finished",
                         streamName_.c_str());
        return false;
    }
    // start decode if status is ok
    if (status_ == DECODE_READY) {
        StartFrameDecoder();
        usleep(kDecodeQueueOpWait);
    }
    // read frame from decode queue
    bool noWait = (status_ == DECODE_DVPP_FINISHED);
    shared_ptr<ImageData> frame = FrameImageOutQueue(noWait);
    if (noWait && (frame == nullptr)) {
        while (!isFrameDecodeEnd_) {
            usleep(kWaitDecodeFinishInterval);
        }
        SetStatus(DECODE_FINISHED);
        LOG_PRINT("[INFO] No frame to read anymore");
        return false;
    }

    if (frame == nullptr) {
        LOG_PRINT("[ERROR] No frame image to read abnormally");
        return false;
    }

    image.format = frame->format;
    image.width = frame->width;
    image.height = frame->height;
    image.alignWidth = frame->alignWidth;
    image.alignHeight = frame->alignHeight;
    image.size = frame->size;
    image.data = frame->data;

    return true;
}

shared_ptr<ImageData> VideoRead::FrameImageOutQueue(bool noWait)
{
    shared_ptr<ImageData> image = frameImageQueue_.Pop();

    if (noWait || (image != nullptr)) return image;

    for (int count = 0; count < kQueueOpRetryTimes - 1; count++) {
        usleep(kDecodeQueueOpWait);

        image = frameImageQueue_.Pop();
        if (image != nullptr)
            return image;
    }

    return nullptr;
}

bool VideoRead::Set(StreamProperty propId, int value)
{
    bool setRet = true;
    Result ret = RES_OK;
    switch (propId) {
        case OUTPUT_IMAGE_FORMAT:
            ret = dvppVdec_->SetFormat(value);
            if (ret != RES_OK) {
                setRet = false;
            }
            break;
        case RTSP_TRANSPORT:
            ret = SetRtspTransType(value);
            if (ret != RES_OK) {
                setRet = false;
            }
            break;
        default:
            setRet = false;
            LOG_PRINT("[ERROR] Unsurpport property %d to set for video %s",
                              (int)propId, streamName_.c_str());
            break;
    }

    return setRet;
}

Result VideoRead::SetRtspTransType(uint32_t transCode)
{
    Result ret = RES_OK;

    if (transCode == RTSP_TRANS_UDP)
        ffmpegDecoder_->SetTransport(RTSP_TRANSPORT_UDP);
    else if (transCode == RTSP_TRANS_TCP)
        ffmpegDecoder_->SetTransport(RTSP_TRANSPORT_TCP);
    else {
        ret = RES_ERROR;
        LOG_PRINT("[ERROR] Unsurport rtsp transport property value %d",
                          transCode);
    }

    return ret;
}

uint32_t VideoRead::Get(StreamProperty propId)
{
    uint32_t value = 0;
    switch (propId) {
        case FRAME_WIDTH:
            value = ffmpegDecoder_->GetFrameWidth();
            break;
        case FRAME_HEIGHT:
            value = ffmpegDecoder_->GetFrameHeight();
            break;
        case VIDEO_FPS:
            value = ffmpegDecoder_->GetFps();
            break;
        default:
            LOG_PRINT("[ERROR] Unsurpport property %d to get for video", propId);
            break;
    }

    return value;
}

Result VideoRead::SetAclContext()
{
    if (context_ == nullptr) {
        LOG_PRINT("[ERROR] Video decoder context is null");
        return RES_ERROR;
    }
    
    aclError ret = aclrtSetCurrentContext(context_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Video decoder set context failed, error: %d", ret);
        return RES_ERROR;
    }
    
    return RES_OK;
}

void VideoRead::Release()
{
    DestroyResource();
    return;
}
}  // namespace acllite