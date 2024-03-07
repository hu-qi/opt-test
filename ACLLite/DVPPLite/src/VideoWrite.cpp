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
#include <cstring>
#include <cstdlib>
#include <malloc.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <fstream>
#include <memory>
#include <thread>
#include <iostream>
#include "acllite_common/Common.h"
#include "VideoWrite.h"

using namespace std;

namespace acllite {
VideoWrite::VideoWrite(string outFile, uint32_t maxWidth,
    uint32_t maxHeight, int32_t deviceId)
    :isReleased_(false), status_(STATUS_VENC_INIT), deviceId_(deviceId),
    dvppVenc_(nullptr), outFile_(outFile),
    maxWidth_(maxWidth), maxHeight_(maxHeight),
    format_(PIXEL_FORMAT_YUV_SEMIPLANAR_420),
    enType_(H264_MAIN_LEVEL)
{
    Open();
}

VideoWrite::~VideoWrite()
{
    DestroyResource();
}

void VideoWrite::DestroyResource()
{
    if (isReleased_) return;
    dvppVenc_->DestroyResource();
    // release dvpp venc
    delete dvppVenc_;
    dvppVenc_ = nullptr;
    isReleased_ = true;
}

Result VideoWrite::InitResource()
{
    dvppVenc_ = new VencHelper(outFile_, maxWidth_, maxHeight_,
        deviceId_, format_, enType_);
    Result ret = dvppVenc_->Init();
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Video encoder init failed");
        return RES_ERROR;
    }
    return RES_OK;
}

Result VideoWrite::Open()
{
    // Init acl resource
    Result ret = InitResource();
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] Init resource failed");
        return RES_ERROR;
    }
    // Set init ok
    string outvideo = outFile_;
    LOG_PRINT("[INFO] Encoded Video %s init ok", outvideo.c_str());
    return RES_OK;
}

// check decoder status
bool VideoWrite::IsOpened()
{
    string outvideo = outFile_;
    status_ = dvppVenc_->GetStatus();
    LOG_PRINT("[INFO] Video %s encode status %d", outvideo.c_str(), status_);
    return (status_ == STATUS_VENC_INIT) || (status_ == STATUS_VENC_WORK);
}

bool VideoWrite::Write(ImageData& image)
{
    Result ret = dvppVenc_->Process(image);
    if (ret != RES_OK) {
        LOG_PRINT("[ERROR] fail to Write encode image");
        return false;
    }
    return true;
}

void VideoWrite::Release()
{
    DestroyResource();
    return;
}

}  // namespace acllite