/**
* @file VdecHelper.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "acllite_common/Common.h"
#include "VdecHelper.h"
using namespace std;

namespace acllite {
    const uint32_t kFrameWidthMax = 4096;
    const uint32_t kFrameHeightMax = 4096;

VdecHelper::VdecHelper(int channelId, uint32_t width, uint32_t height,
    int type, aclvdecCallback callback, uint32_t outFormat)
    :channelId_(channelId), format_(outFormat), enType_(type),
    frameWidth_(width), frameHeight_(height), callback_(callback),
    isExit_(false), isReleased_(false)
{
    vdecChannelDesc_ = nullptr;
    inputStreamDesc_ = nullptr;
    outputPicDesc_ = nullptr;
    outputPicBuf_ = nullptr;
    aclError aclRet;
    LOG_PRINT("[INFO] get current context");
    aclRet = aclrtGetCurrentContext(&context_);
    if ((aclRet != ACL_SUCCESS) || (context_ == nullptr)) {
        LOG_PRINT("[ERROR] VdecHelper : Get current acl context error:%d", aclRet);
    }
    LOG_PRINT("[INFO] VDEC width %d, height %d", frameWidth_, frameHeight_);
}

VdecHelper::~VdecHelper()
{
    DestroyResource();
}

void VdecHelper::DestroyResource()
{
    if (isReleased_) {
        return;
    }

    aclError ret;
    if (inputStreamDesc_ != nullptr) {
        void* inputBuf = acldvppGetStreamDescData(inputStreamDesc_);
        if (inputBuf != nullptr) {
            acldvppFree(inputBuf);
        }
        aclError ret = acldvppDestroyStreamDesc(inputStreamDesc_);
        if (ret != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] fail to destroy input stream desc");
        }
        inputStreamDesc_ = nullptr;
    }

    if (outputPicDesc_ != nullptr) {
        void* outputBuf = acldvppGetPicDescData(outputPicDesc_);
        if (outputBuf != nullptr) {
            acldvppFree(outputBuf);
        }
        aclError ret = acldvppDestroyPicDesc(outputPicDesc_);
        if (ret != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] fail to destroy output pic desc");
        }
        outputPicDesc_ = nullptr;
    }

    if (vdecChannelDesc_ != nullptr) {
        ret = aclvdecDestroyChannel(vdecChannelDesc_);
        if (ret != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] Vdec destroy channel failed, errorno: %d", ret);
        }
        aclvdecDestroyChannelDesc(vdecChannelDesc_);
        vdecChannelDesc_ = nullptr;
    }

    UnsubscribReportThread();

    if (stream_ != nullptr) {
        ret = aclrtDestroyStream(stream_);
        if (ret != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] Vdec destroy stream failed");
        }
        stream_ = nullptr;
    }

    isReleased_ = true;
}

void* VdecHelper::SubscribeReportThreadFunc(void *arg)
{
    LOG_PRINT("[INFO] Start vdec subscribe thread...");

    // Notice: create context for this thread
    VdecHelper* vdec = (VdecHelper *)arg;
    aclrtContext context = vdec->GetContext();
    aclError ret = aclrtSetCurrentContext(context);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Video decoder set context failed, error: %d", ret);
    }

    while (!vdec->IsExit()) {
        // Notice: timeout 1000ms
        aclrtProcessReport(1000);
    }

    LOG_PRINT("[INFO] Vdec subscribe thread exit!");

    return (void*)RES_OK;
}

void VdecHelper::UnsubscribReportThread()
{
    if ((subscribeThreadId_ == 0) || (stream_ == nullptr)) return;

    (void)aclrtUnSubscribeReport(static_cast<uint64_t>(subscribeThreadId_),
                                 stream_);
    // destory thread
    isExit_ = true;

    void *res = nullptr;
    int joinThreadErr = pthread_join(subscribeThreadId_, &res);
    if (joinThreadErr) {
        LOG_PRINT("[ERROR] Join thread failed, threadId = %lu, err = %d",
                          subscribeThreadId_, joinThreadErr);
    } else {
        if ((uint64_t)res != 0) {
            LOG_PRINT("[ERROR] thread run failed. ret is %lu.", (uint64_t)res);
        }
    }
    LOG_PRINT("[INFO] Destory report thread success.");
}

Result VdecHelper::Init()
{
    LOG_PRINT("[INFO] Vdec process init start...");
    aclError aclRet = aclrtCreateStream(&stream_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Vdec create stream failed, errorno:%d", aclRet);
        return RES_ERROR;
    }
    LOG_PRINT("[INFO] Vdec create stream ok");

    int ret = pthread_create(&subscribeThreadId_, nullptr,
                             SubscribeReportThreadFunc, (void *)this);
    if (ret) {
        LOG_PRINT("[ERROR] Start vdec subscribe thread failed, return:%d", ret);
        return RES_ERROR;
    }
    (void)aclrtSubscribeReport(static_cast<uint64_t>(subscribeThreadId_),
                               stream_);

    ret = CreateVdecChannelDesc();
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Create vdec channel failed");
        return ret;
    }

    return RES_OK;
}

Result VdecHelper::CreateVdecChannelDesc()
{
    vdecChannelDesc_ = aclvdecCreateChannelDesc();
    if (vdecChannelDesc_ == nullptr) {
        LOG_PRINT("[ERROR] Create vdec channel desc failed");
        return RES_ERROR;
    }

   // channelId: 0-15
    aclError ret = aclvdecSetChannelDescChannelId(vdecChannelDesc_,
                                                  channelId_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Set vdec channel id to %d failed, errorno:%d",
                          channelId_, ret);
        return RES_ERROR;
    }

    ret = aclvdecSetChannelDescThreadId(vdecChannelDesc_, subscribeThreadId_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Set vdec channel thread id failed, errorno:%d", ret);
        return RES_ERROR;
    }

    // callback func
    ret = aclvdecSetChannelDescCallback(vdecChannelDesc_, callback_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Set vdec channel callback failed, errorno:%d", ret);
        return RES_ERROR;
    }

    ret = aclvdecSetChannelDescEnType(vdecChannelDesc_,
                                      static_cast<acldvppStreamFormat>(enType_));
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Set vdec channel entype failed, errorno:%d", ret);
        return RES_ERROR;
    }

    ret = aclvdecSetChannelDescOutPicFormat(vdecChannelDesc_,
                                            static_cast<acldvppPixelFormat>(format_));
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Set vdec channel pic format failed, errorno:%d", ret);
        return RES_ERROR;
    }

    // create vdec channel
    LOG_PRINT("[INFO] Start create vdec channel by desc...");
    ret = aclvdecCreateChannel(vdecChannelDesc_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] fail to create vdec channel");
        return RES_ERROR;
    }
    LOG_PRINT("[INFO] Create vdec channel ok");

    return RES_OK;
}

Result VdecHelper::CreateInputStreamDesc(shared_ptr<FrameData> frameData)
{
    inputStreamDesc_ = acldvppCreateStreamDesc();
    CHECK_RET(inputStreamDesc_ != nullptr, LOG_PRINT("[ERROR] Create input stream desc failed"); return RES_ERROR);
    aclError ret;
    // to the last data,send an endding signal to dvpp vdec
    if (frameData->isFinished) {
        ret = acldvppSetStreamDescEos(inputStreamDesc_, 1);
        CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set EOS to input stream desc failed, errorno:%d", ret); return RES_ERROR);
        return RES_OK;
    }

    ret = acldvppSetStreamDescData(inputStreamDesc_, frameData->data);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set input stream data failed, errorno:%d", ret); return RES_ERROR);
    // set size for dvpp stream desc
    ret = acldvppSetStreamDescSize(inputStreamDesc_, frameData->size);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set input stream size failed, errorno:%d", ret); return RES_ERROR);
    acldvppSetStreamDescTimestamp(inputStreamDesc_, frameData->frameId);

    return RES_OK;
}

Result VdecHelper::CreateOutputPicDesc()
{
    frameWidth_ = ALIGN_UP2(frameWidth_);
    frameHeight_ = ALIGN_UP2(frameHeight_);
    alignWidth_ = ALIGN_UP16(frameWidth_);
    alignHeight_ = ALIGN_UP2(frameHeight_);
    outputPicSize_ = YUV420SP_SIZE(alignWidth_, alignHeight_);
    // Malloc output device memory
    aclError ret = acldvppMalloc(&outputPicBuf_, outputPicSize_);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Malloc vdec output buffer failed when create "
              "vdec output desc, errorno:%d", ret); return RES_ERROR);
    outputPicDesc_ = acldvppCreatePicDesc();
    if (outputPicDesc_ == nullptr) {
        LOG_PRINT("[ERROR] Create vdec output pic desc failed");
        return RES_ERROR;
    }
    ret = acldvppSetPicDescData(outputPicDesc_, outputPicBuf_);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set vdec output pic desc data failed, errorno:%d", ret); return RES_ERROR);
    ret = acldvppSetPicDescSize(outputPicDesc_, outputPicSize_);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set vdec output pic size failed, errorno:%d", ret); return RES_ERROR);
    ret = acldvppSetPicDescWidth(outputPicDesc_, frameWidth_);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set vdec output pic width failed, errorno:%d", ret); return RES_ERROR);
    ret = acldvppSetPicDescHeight(outputPicDesc_, frameHeight_);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set vdec output pic height failed, errorno:%d", ret); return RES_ERROR);
    ret = acldvppSetPicDescWidthStride(outputPicDesc_, alignWidth_);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set vdec output pic widthStride failed, errorno:%d", ret); return RES_ERROR);
    ret = acldvppSetPicDescHeightStride(outputPicDesc_, alignHeight_);
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set vdec output pic heightStride failed, errorno:%d", ret); return RES_ERROR);
    ret = acldvppSetPicDescFormat(outputPicDesc_, static_cast<acldvppPixelFormat>(format_));
    CHECK_RET(ret == ACL_SUCCESS, LOG_PRINT("[ERROR] Set vdec output pic format failed, errorno:%d", ret); return RES_ERROR);
    return RES_OK;
}

Result VdecHelper::Process(shared_ptr<FrameData> frameData, void* userData)
{
    // create input desc
    Result atlRet = CreateInputStreamDesc(frameData);
    if (atlRet != RES_OK) {
        LOG_PRINT("[ERROR] Create stream desc failed");
        return atlRet;
    }

    if (!frameData->isFinished) {
        // create out desc
        atlRet = CreateOutputPicDesc();
        if (atlRet != RES_OK) {
            LOG_PRINT("[ERROR] Create pic desc failed");
            return atlRet;
        }
    } else {
        outputPicDesc_ = acldvppCreatePicDesc();
        if (outputPicDesc_ == nullptr) {
            LOG_PRINT("[ERROR] Create vdec output pic desc failed");
            return RES_ERROR;
        }
    }

    // send data to dvpp vdec to decode
    aclError ret = aclvdecSendFrame(vdecChannelDesc_, inputStreamDesc_,
                                    outputPicDesc_, nullptr, userData);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Send frame to vdec failed, errorno:%d", ret);
        return RES_ERROR;
    }

    return RES_OK;
}

Result VdecHelper::SetFormat(uint32_t format)
{
    if ((format != PIXEL_FORMAT_YUV_SEMIPLANAR_420) ||
        (format != PIXEL_FORMAT_YVU_SEMIPLANAR_420)) {
        LOG_PRINT("[ERROR] Set video decode output image format to %d failed, "
            "only support %d(YUV420SP NV12) and %d(YUV420SP NV21)",
            format,
            (int)PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            (int)PIXEL_FORMAT_YVU_SEMIPLANAR_420);
        return RES_ERROR;
    }

    format_ = format;
    LOG_PRINT("[INFO] Set video decode output image format to %d ok", format);

    return RES_OK;
}

Result VdecHelper::VideoParamCheck()
{
    if ((frameWidth_ == 0) || (frameWidth_ > kFrameWidthMax)) {
        LOG_PRINT("[ERROR] video frame width %d is invalid, the legal range is [0, %d]",
                          frameWidth_, kFrameWidthMax);
        return RES_ERROR;
    }
    if ((frameHeight_ == 0) || (frameHeight_ > kFrameHeightMax)) {
        LOG_PRINT("[ERROR] video frame height %d is invalid, the legal range is [0, %d]",
                          frameHeight_, kFrameHeightMax);
        return RES_ERROR;
    }
    if ((format_ != PIXEL_FORMAT_YUV_SEMIPLANAR_420) &&
        (format_ != PIXEL_FORMAT_YVU_SEMIPLANAR_420)) {
        LOG_PRINT("[ERROR] video decode image format %d invalid, "
                          "only support %d(YUV420SP NV12) and %d(YUV420SP NV21)",
                          format_, (int)PIXEL_FORMAT_YUV_SEMIPLANAR_420,
                          (int)PIXEL_FORMAT_YVU_SEMIPLANAR_420);
        return RES_ERROR;
    }
    if (enType_ > (uint32_t)H264_HIGH_LEVEL) {
        LOG_PRINT("[ERROR] Input video stream format %d invalid", enType_);
        return RES_ERROR;
    }

    return RES_OK;
}
}  // namespace acllite