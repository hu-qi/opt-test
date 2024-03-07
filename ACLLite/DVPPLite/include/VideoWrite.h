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

#ifndef VIDEO_WRITE_H
#define VIDEO_WRITE_H

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
#include "VencHelper.h"

namespace acllite {

class VideoWrite {
public:
    /**
     * @brief VideoWrite constructor
     */
    VideoWrite(std::string outFile, uint32_t maxWidth,
        uint32_t maxHeight, int32_t deviceId = 0);
    /**
     * @brief VideoWrite destructor
     */
    ~VideoWrite();
    bool IsOpened();
    bool Write(ImageData& image);
    void Release();

private:
    Result Open();
    Result InitResource();
    void DestroyResource();

private:
    bool isReleased_;
    VencStatus status_;
    int32_t deviceId_;
    VencHelper* dvppVenc_;
    std::string outFile_;
    uint32_t maxWidth_;
    uint32_t maxHeight_;
    acldvppPixelFormat format_;
    acldvppStreamFormat enType_;
};
}  // namespace acllite
#endif /* VIDEO_WRITE_H */
