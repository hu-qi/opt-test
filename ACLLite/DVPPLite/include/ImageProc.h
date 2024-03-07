/**
* Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

* File ImageProc.h
* Description: handle dvpp process
*/
#ifndef IMAGEPROC_H
#define IMAGEPROC_H
#pragma once
#include "acllite_common/Common.h"
#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"

namespace acllite {
class ImageProc {
public:
    ImageProc(int deviceId = 0);
    ~ImageProc();
    ImageData Read(const std::string& fileName, acldvppPixelFormat imgFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    void Resize(ImageData& src, ImageData& dst, ImageSize dsize, ResizeType type = RESIZE_COMMON);
    void Crop(ImageData& src, ImageData& dst, DvRect dRect, ImageSize dSize);
    bool Write(const std::string& fileName, ImageData& img);
private:
    bool Init();
    void DestroyResource();
    bool SetJpegdPicDescNV12(uint32_t srcWidth, uint32_t srcHeight, acldvppPixelFormat dstFormat);
    ImageData JpegD(void*& hostData, uint32_t size, acldvppPixelFormat dstFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    bool SetPngdPicDescRGB(uint32_t srcWidth, uint32_t srcHeight, acldvppPixelFormat dstFormat);
    ImageData PngD(void*& hostData, uint32_t size, acldvppPixelFormat dstFormat = PIXEL_FORMAT_RGB_888);
    bool SetVpcInputPicDescYUV420SP(ImageData& src);
    bool SetVpcInputPicDescYUV422P(ImageData& src);
    bool SetVpcOutputPicDescYUV420SP(ImageSize dsize, acldvppPixelFormat format);
    void ResizeCommon(ImageData& src, ImageData& dst, ImageSize dsize);
    void ResizeProportionalUpperLeft(ImageData& src, ImageData& dst, ImageSize dsize);
    void ResizeProportionalCenter(ImageData& src, ImageData& dst, ImageSize dsize);
    bool SetJpgePicDescNV12(ImageData& src);
    bool AlignSpImage(const ImageData &picDesc, uint8_t *inBuffer, uint8_t *imageBuffer);

private:
    bool isReleased_;
    aclrtStream stream_;
    acldvppChannelDesc *dvppChannelDesc_;
    acldvppJpegeConfig* jpegeConfig_;
    acldvppResizeConfig* resizeConfig_;
    acldvppRoiConfig* cropArea_;
    acldvppRoiConfig* pasteArea_;
    acldvppPicDesc* srcPicDesc_;
    uint32_t srcBufferSize_;
    uint32_t srcWidth_;
    uint32_t srcHeight_;
    uint32_t srcWidthStride_;
    uint32_t srcHeightStride_;
    acldvppPicDesc* dstPicDesc_;
    void* dstBuffer_;
    uint32_t dstBufferSize_;
    uint32_t dstWidth_;
    uint32_t dstHeight_;
    uint32_t dstWidthStride_;
    uint32_t dstHeightStride_;
    std::string socVersion_;
    size_t nPos_;
    size_t pos310P_;
    size_t pos310B_;
    size_t pos910B_;
};
}  // namespace acllite
#endif