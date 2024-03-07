/**
* @file VdecHelper.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef VDEC_PROCESS_H
#define VDEC_PROCESS_H

#include <cstdint>
#include <iostream>
#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
namespace acllite {
class VdecHelper {
public:
    VdecHelper(int channel, uint32_t width, uint32_t height,
                int type, aclvdecCallback callback,
                uint32_t outFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    ~VdecHelper();

    static void* SubscribeReportThreadFunc(void *arg);

    Result Init();
    void DestroyResource();
    Result Process(std::shared_ptr<FrameData> frameData, void* userData);
    Result SetFormat(uint32_t format);
    Result VideoParamCheck();
    bool IsExit()
    {
        return isExit_;
    }
    aclrtContext GetContext()
    {
        return context_ ;
    }

private:
    Result CreateVdecChannelDesc();
    Result CreateInputStreamDesc(std::shared_ptr<FrameData> frame);
    Result CreateOutputPicDesc();
    void UnsubscribReportThread();

private:
    int channelId_;
    uint32_t format_;
    uint32_t enType_;

    uint32_t frameWidth_;
    uint32_t frameHeight_;
    uint32_t alignWidth_;
    uint32_t alignHeight_;
    uint32_t outputPicSize_;
    void *outputPicBuf_;
    aclvdecCallback callback_;
    aclrtContext context_;
    aclrtStream stream_;

    aclvdecChannelDesc *vdecChannelDesc_;
    acldvppStreamDesc *inputStreamDesc_;
    acldvppPicDesc *outputPicDesc_;

    pthread_t subscribeThreadId_;
    bool isExit_;
    bool isReleased_;
};
}  // namespace acllite
#endif
