/**
* @file VencHelper.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef VENC_HELPER_H
#define VENC_HELPER_H
#pragma once
#include <cstdint>
#include <iostream>
#include <thread>
#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
#include "acllite_common/Common.h"
#include "acllite_common/ThreadSafeQueue.h"

namespace acllite {
class VencHelper {
public:
    VencHelper(std::string outFile, uint32_t maxWidth,
        uint32_t maxHeight, int32_t deviceId,
        acldvppPixelFormat format, acldvppStreamFormat enType);
    ~VencHelper();
    Result Init();
    Result Process(ImageData &image);
    void Finish();
    VencStatus GetStatus()
    {
        return status_;
    }
    
    void DestroyResource();
private:
    void SetStatus(VencStatus status)
    {
        status_ = status;
    }
    Result CreateVencChannel();
    Result CreateInputPicDesc(ImageData& image);
    Result CreateFrameConfig();
    Result SetFrameConfig(uint8_t eos, uint8_t forceIFrame);
    Result SaveVencFile(void* vencData, uint32_t size);

    static void Callback(acldvppPicDesc *input,
                         acldvppStreamDesc *output, void *userData);
    static void* SubscribleThreadFunc(void *arg);

private:
    VencStatus status_;
    aclvencChannelDesc *vencChannelDesc_;
    aclvencFrameConfig *vencFrameConfig_;
    aclrtStream vencStream_;
    acldvppPicDesc *inputPicDesc_;
    FILE *outFp_;
    bool isFinished_;
    pthread_t threadId_;
    uint32_t finFrameCnt_;
    uint32_t frameId_;
    bool isReleased_;

    int32_t deviceId_;
    aclrtContext context_;
    aclrtRunMode runMode_;
    std::string outFile_;
    uint32_t maxWidth_;
    uint32_t maxHeight_;
    acldvppPixelFormat format_;
    acldvppStreamFormat enType_;

};
}  // namespace acllite
#endif