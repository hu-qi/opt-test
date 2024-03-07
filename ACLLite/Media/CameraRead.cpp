#include <memory>
#include <stdio.h>
extern "C" {
#include <fcntl.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}
#include "CameraRead.h"
#include "acllite_common/Common.h"

using namespace std;

namespace acllite {
const int kWait = 10000;
CameraRead::CameraRead(const std::string& videoName, int32_t deviceId, aclrtContext context)
    :deviceId_(deviceId), context_(context), streamName_(videoName), formatContext_(nullptr),
    isReleased_(false), isOpened_(false), isStop_(false),
    isStarted_(false), frameQueue_(10)
{
    Open();
}

CameraRead::~CameraRead()
{
    DestroyResource();
}

void CameraRead::DestroyResource()
{
    if (isReleased_) return;
    isOpened_ = false;
    isReleased_ = true;
    return;
}

bool CameraRead::IsOpened()
{
    return isOpened_;
}

uint32_t CameraRead::Get(StreamProperty propId)
{
    uint32_t value = 0;
    switch (propId) {
        case FRAME_WIDTH:
            value = width_;
            break;
        case FRAME_HEIGHT:
            value = height_;
            break;
        case VIDEO_FPS:
            value = fps_;
            break;
        default:
            LOG_PRINT("[ERROR] Unsurpport property %d to get for video", propId);
            break;
    }
    return value;
}

bool CameraRead::Read(ImageData& frame)
{
    if (!isOpened_) {
        return false;
    }

    if (!isStarted_) {
        StartFrameDecoder();
        usleep(1000);
        isStarted_ = true;
    }
    shared_ptr<ImageData> image = frameQueue_.Pop();
    if (image != nullptr) {
        frame.data = image->data;
        frame.width = image->width;
        frame.height = image->height;
        frame.format = image->format;
        frame.size = image->size;
        return true;
    }
    for (int count = 0; count < kWait - 1; count++) {
        usleep(1000);
        image = frameQueue_.Pop();
        if (image != nullptr) {
            frame.data = image->data;
            frame.width = image->width;
            frame.height = image->height;
            frame.format = image->format;
            frame.size = image->size;
            return true;
        }
    }
    return false;
}

void CameraRead::Release()
{
    isStop_ = true;
    DestroyResource();
    return;
}

bool CameraRead::SendFrame(void* frameData, int frameSize) {
    // frameId_++  record framenum;
    if ((frameData == NULL) || (frameSize == 0)) {
        LOG_PRINT("Frame data is null");
        return false;
    }
    aclError aclRet;
    // use current thread context default
    if (context_ == nullptr) {
        aclRet = aclrtCreateContext(&context_, deviceId_);
        if ((aclRet != ACL_SUCCESS) || (context_ == nullptr)) {
            LOG_PRINT("[ERROR] Get current acl context error:%d", aclRet);
            return false;
        }
    }
    void* deviceMem = nullptr;
    aclRet = acldvppMalloc(&deviceMem, frameSize);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("acldvppMalloc device for usbcamera failed. ERROR: %d", aclRet); return false);
    // copy to device
    aclRet = aclrtMemcpy(deviceMem, frameSize, frameData, frameSize, ACL_MEMCPY_HOST_TO_DEVICE);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("aclrtMemcpy failed. ERROR: %d", aclRet);
        acldvppFree(deviceMem);
        return false;
    }
    shared_ptr<ImageData> videoFrame = make_shared<ImageData>();
    videoFrame->data = SHARED_PTR_DEV_BUF(deviceMem);
    videoFrame->size = frameSize;
    videoFrame->width = width_;
    videoFrame->height = height_;
    videoFrame->format = PIXEL_FORMAT_YUYV_PACKED_422;
    frameQueue_.Push(videoFrame);
    return true;
}

void CameraRead::DecodeFrameThread(void* decoderSelf)
{
    CameraRead* thisPtr = (CameraRead*)decoderSelf;
    int videoStreamIndex = -1;
    for (int i = 0; i < thisPtr->formatContext_->nb_streams; ++i) {
        if (thisPtr->formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        LOG_PRINT("[ERROR] usb camera %s index is -1", thisPtr->streamName_.c_str());
        thisPtr->isOpened_ = false;
        return;
    }
    AVCodecParameters* codecParameters = thisPtr->formatContext_->streams[videoStreamIndex]->codecpar;
    AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        LOG_PRINT("[ERROR] Could not find ffmpeg decoder.");
        thisPtr->isOpened_ = false;
        return;
    }
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        LOG_PRINT("[ERROR] Could not create decoder context.");
        thisPtr->isOpened_ = false;
        return;
    }
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        LOG_PRINT("[ERROR] Could not open decoder context.");
        thisPtr->isOpened_ = false;
        return;
    }
    AVFrame* frame = av_frame_alloc();
    AVPacket packet;

    while (av_read_frame(thisPtr->formatContext_, &packet) >= 0 && !thisPtr->isStop_) {
        if (packet.stream_index == videoStreamIndex) {
            int response = avcodec_send_packet(codecContext, &packet);
            if (response < 0 || response == AVERROR(EAGAIN)) {
                continue;
            }
            while (response >= 0) {
                response = avcodec_receive_frame(codecContext, frame);
                if (response == AVERROR(EAGAIN)) {
                    break;
                } else if (response < 0) {
                    LOG_PRINT("[ERROR] Receive false frame from ffmpeg.");
                    thisPtr->isOpened_ = false;
                    return;
                }
                bool ret = thisPtr->SendFrame(packet.data, packet.size);
                if (!ret) {
                    thisPtr->isOpened_ = false;
                    LOG_PRINT("[ERROR] Send single frame from ffmpeg failed.");
                    return;
                }
            }
        }
        av_packet_unref(&packet);
    }
    av_frame_free(&frame);
    avcodec_close(codecContext);
    avformat_close_input(&thisPtr->formatContext_);
    thisPtr->isOpened_ = false;
    return;
}

void CameraRead::StartFrameDecoder()
{
    decodeThread_ = thread(DecodeFrameThread, (void*)this);
    decodeThread_.detach();
}

Result CameraRead::Open()
{   
    avdevice_register_all();
	AVDictionary *options = NULL;
	AVInputFormat *iformat = av_find_input_format("v4l2");
	av_dict_set(&options,"video_size","1280*720",0);
	av_dict_set(&options,"framerate","10",0);
	av_dict_set(&options,"pixel_format","yuv420p",0);
    width_ = 1280;
    height_ = 720;
    fps_ = 10;
    if (int err_code = avformat_open_input(&formatContext_, streamName_.c_str(), iformat, &options)) {
        LOG_PRINT("[ERROR] Could not open video:%s, return :%d", streamName_.c_str(), err_code);
        isOpened_ = false;
        return RES_ERROR;
    }
    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        LOG_PRINT("[ERROR] Get stream info of %s failed", streamName_.c_str());
        isOpened_ = false;
        return RES_ERROR;
    }
    isOpened_ = true;
    return RES_OK;
}

}  // namespace acllite