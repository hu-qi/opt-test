/**
* @file VencHelper.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include <cstdint>
#include <iostream>
#include <cerrno>
#include <cstring>

#include "VencHelper.h"

using namespace std;
namespace acllite {
    uint32_t kKeyFrameInterval = 16;
    uint32_t kRcMode = 2;
    uint32_t kMaxBitRate = 10000;
    uint32_t kVencQueueSize = 256;
    uint32_t kImageEnQueueRetryTimes = 3;
    uint32_t kEnqueueWait = 10000;
    uint32_t kOutqueueWait = 10000;
    uint32_t kAsyncWait = 10000;
    bool g_runFlag = true;

VencHelper::VencHelper(string outFile, uint32_t maxWidth,
    uint32_t maxHeight, int32_t deviceId,
    acldvppPixelFormat format, acldvppStreamFormat enType)
    :status_(STATUS_VENC_INIT), vencChannelDesc_(nullptr),
    vencFrameConfig_(nullptr), vencStream_(nullptr),
    inputPicDesc_(nullptr), outFp_(nullptr), 
    isFinished_(false), threadId_(0),
    finFrameCnt_(0), frameId_(0), isReleased_(false),
    outFile_(outFile), maxWidth_(maxWidth), maxHeight_(maxHeight),
    format_(format), enType_(enType), context_(nullptr)
{
}

VencHelper::~VencHelper()
{
    DestroyResource();
}

Result VencHelper::SaveVencFile(void* vencData, uint32_t size)
{
    Result ret = RES_OK;
    void* hostData = vencData;
    if (runMode_ == ACL_HOST) {
        aclError aclRet = aclrtMallocHost(&hostData, size);
        CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtMallocHost failed. ERROR: %d", aclRet); return false);
        // copy to host
        aclRet = aclrtMemcpy(hostData, size, vencData, size, ACL_MEMCPY_DEVICE_TO_HOST);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
            aclrtFreeHost(hostData);
            return false;
        }
    }
    size_t checkSize = fwrite(hostData, 1, size, outFp_);
    if (checkSize != size) {
        LOG_PRINT("[ERROR] Save venc file %s failed, need write %u bytes, "
                          "but only write %zu bytes, error: %s",
                          outFile_.c_str(), size, checkSize, strerror(errno));
        ret = RES_ERROR;
    } else {
        fflush(outFp_);
    }

    if (runMode_ == ACL_HOST) {
        aclrtFreeHost(hostData);
    }

    return ret;
}

void VencHelper::Callback(acldvppPicDesc *input,
                          acldvppStreamDesc *output, void *userData)
{
    VencHelper* venc = (VencHelper*)userData;
    void* data = acldvppGetStreamDescData(output);
    uint32_t retCode = acldvppGetStreamDescRetCode(output);
    if (retCode == 0) {
        // encode success, then process output pic
        uint32_t size = acldvppGetStreamDescSize(output);
        Result ret = venc->SaveVencFile(data, size);
        if (ret != RES_OK) {
            LOG_PRINT("[ERROR] Save venc file failed, error %d", ret);
        } else {
            LOG_PRINT("[INFO] success to callback, stream size:%u", size);
        }
    } else {
        LOG_PRINT("[ERROR] venc encode frame failed, ret = %u.", retCode);
    }
    void* inputData = acldvppGetPicDescData(input);
    if (inputData!=nullptr) {
        acldvppFree(inputData);
    }
    acldvppDestroyPicDesc(input);
    venc->finFrameCnt_++;
}

void* VencHelper::SubscribleThreadFunc(aclrtContext sharedContext)
{
    if (sharedContext == nullptr) {
        LOG_PRINT("[ERROR] sharedContext can not be nullptr");
        return ((void*)(-1));
    }
    LOG_PRINT("[INFO] use shared context for this thread");
    aclError ret = aclrtSetCurrentContext(sharedContext);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] aclrtSetCurrentContext failed, errorCode = %d", static_cast<int32_t>(ret));
        return ((void*)(-1));
    }

    while (g_runFlag) {
        // Notice: timeout 1000ms
        (void)aclrtProcessReport(1000);
    }

    return (void*)0;
}

Result VencHelper::CreateVencChannel()
{
    // create vdec channelDesc
    vencChannelDesc_ = aclvencCreateChannelDesc();
    if (vencChannelDesc_ == nullptr) {
        LOG_PRINT("[ERROR] Create venc channel desc failed");
        return RES_ERROR;
    }

    aclvencSetChannelDescThreadId(vencChannelDesc_, threadId_);
    aclvencSetChannelDescCallback(vencChannelDesc_, &VencHelper::Callback);
    aclvencSetChannelDescEnType(vencChannelDesc_, enType_);
    aclvencSetChannelDescPicFormat(vencChannelDesc_, format_);
    aclvencSetChannelDescPicWidth(vencChannelDesc_, maxWidth_);
    aclvencSetChannelDescPicHeight(vencChannelDesc_, maxHeight_);
    aclvencSetChannelDescKeyFrameInterval(vencChannelDesc_, kKeyFrameInterval);
    aclvencSetChannelDescRcMode(vencChannelDesc_, kRcMode);
    aclvencSetChannelDescMaxBitRate(vencChannelDesc_, kMaxBitRate);
    aclError ret = aclvencCreateChannel(vencChannelDesc_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] fail to create venc channel");
        return RES_ERROR;
    }
    return RES_OK;
}

Result VencHelper::CreateFrameConfig()
{
    vencFrameConfig_ = aclvencCreateFrameConfig();
    if (vencFrameConfig_ == nullptr) {
        LOG_PRINT("[ERROR] Create frame config");
        return RES_ERROR;
    }

    Result ret = SetFrameConfig(0, 1);
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Set frame config failed, error %d", ret);
        return ret;
    }

    return RES_OK;
}

Result VencHelper::SetFrameConfig(uint8_t eos, uint8_t forceIFrame)
{
    // set eos
    aclError ret = aclvencSetFrameConfigEos(vencFrameConfig_, eos);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] fail to set eos, ret = %d", ret);
        return RES_ERROR;
    }

    ret = aclvencSetFrameConfigForceIFrame(vencFrameConfig_, forceIFrame);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] fail to set venc ForceIFrame");
        return RES_ERROR;
    }

    return RES_OK;
}

Result VencHelper::Init()
{
    if (status_ != STATUS_VENC_INIT) {
        return RES_ERROR;
    }
    outFp_ = fopen(outFile_.c_str(), "wb+");
    if (outFp_ == nullptr) {
        LOG_PRINT("[ERROR] Open file %s failed, error %s",
                          outFile_.c_str(), strerror(errno));
        return RES_ERROR;
    }

    aclError aclRet;
    // Get current run mode
    aclRet = aclrtGetRunMode(&runMode_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] acl get run mode failed");
        return RES_ERROR;
    }

    if (context_ == nullptr) {
        aclRet = aclrtGetCurrentContext(&context_);
        if ((aclRet != ACL_SUCCESS) || (context_ == nullptr)) {
            LOG_PRINT("[ERROR] Get current acl context error:%d", aclRet);
            return RES_ERROR;
        }
    }
    aclRet = aclrtSetCurrentContext(context_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Video decoder set context failed, error: %d", aclRet);
        return RES_ERROR;
    }

    // create process callback thread
    Result ret = pthread_create(&threadId_, nullptr,
    &VencHelper::SubscribleThreadFunc, context_);
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Create venc subscrible thread failed, error %d", ret);
        return RES_ERROR;
    }

    ret = CreateVencChannel();
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Create venc channel failed, error %d", ret);
        return ret;
    }

    aclRet = aclrtCreateStream(&vencStream_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Create venc stream failed, error %d", aclRet);
        return RES_ERROR;
    }

    aclRet = aclrtSubscribeReport(threadId_, vencStream_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Venc subscrible report failed, error %d", aclRet);
        return RES_ERROR;
    }

    ret = CreateFrameConfig();
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Create venc frame config failed, error %d", ret);
        return ret;
    }
    SetStatus(STATUS_VENC_WORK);
    LOG_PRINT("[INFO] venc init resource success");
    return RES_OK;
}

Result VencHelper::CreateInputPicDesc(ImageData& image)
{
    inputPicDesc_ = acldvppCreatePicDesc();
    if (inputPicDesc_ == nullptr) {
        LOG_PRINT("[ERROR] Create input pic desc failed");
        return RES_ERROR;
    }
    void* deviceMem = nullptr;
    aclError aclRet = acldvppMalloc(&deviceMem, image.size);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppMalloc failed. ERROR: %d", aclRet); return RES_ERROR);
    // copy to device
    aclRet = aclrtMemcpy(deviceMem, image.size, image.data.get(), image.size, ACL_MEMCPY_DEVICE_TO_DEVICE);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
        acldvppFree(deviceMem);
        return RES_ERROR;
    }
    acldvppSetPicDescFormat(inputPicDesc_, format_);
    acldvppSetPicDescWidth(inputPicDesc_, image.width);
    acldvppSetPicDescHeight(inputPicDesc_, image.height);
    acldvppSetPicDescWidthStride(inputPicDesc_, ALIGN_UP16(image.width));
    acldvppSetPicDescHeightStride(inputPicDesc_, ALIGN_UP2(image.height));
    acldvppSetPicDescData(inputPicDesc_, deviceMem);
    acldvppSetPicDescSize(inputPicDesc_, image.size);

    return RES_OK;
}

Result VencHelper::Process(ImageData& image)
{
    if (status_ != STATUS_VENC_WORK) {
        LOG_PRINT("[ERROR] The venc(status %d) is not working", status_);
        return RES_ERROR;
    }
    // create picture desc
    Result ret = CreateInputPicDesc(image);
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] fail to create picture description");
        return ret;
    }

    // send frame
    acldvppStreamDesc *outputStreamDesc = nullptr;

    ret = aclvencSendFrame(vencChannelDesc_, inputPicDesc_,
        static_cast<void *>(outputStreamDesc), vencFrameConfig_, (void *)this);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] send venc frame failed, error %d", ret);
        return RES_ERROR;
    }
    frameId_++;
    return RES_OK;
}

void VencHelper::Finish()
{
    if (isFinished_) {
        return;
    }
    // set frame config, eos frame
    Result ret = SetFrameConfig(1, 0);
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Set eos frame config failed, error %d", ret);
        return;
    }
    // send eos frame
    ret = aclvencSendFrame(vencChannelDesc_, nullptr,
                           nullptr, vencFrameConfig_, nullptr);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] fail to send eos frame, ret=%u", ret);
        return;
    }
    fclose(outFp_);
    outFp_ = nullptr;
    while (finFrameCnt_ < frameId_) {
        usleep(100);
    }
    isFinished_ = true;
    LOG_PRINT("[INFO] venc process success");
    return;
}

void VencHelper::DestroyResource()
{
    if (isReleased_) {
        return;
    }
    
    Finish();
    if (vencChannelDesc_ != nullptr) {
        (void)aclvencDestroyChannel(vencChannelDesc_);
        (void)aclvencDestroyChannelDesc(vencChannelDesc_);
        vencChannelDesc_ = nullptr;
    }
    if (inputPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(inputPicDesc_);
        inputPicDesc_ = nullptr;
    }
    if (vencStream_ != nullptr) {
        aclError ret = aclrtDestroyStream(vencStream_);
        if (ret != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] Vdec destroy stream failed, error %d", ret);
        }
        vencStream_ = nullptr;
    }

    if (vencFrameConfig_ != nullptr) {
        (void)aclvencDestroyFrameConfig(vencFrameConfig_);
        vencFrameConfig_ = nullptr;
    }
    isReleased_ = true;
}

}  // namespace acllite
