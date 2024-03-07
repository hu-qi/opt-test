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

* File ImageProc.cpp
* Description: handle dvpp process
*/
#include <iostream>
#include <cstdlib>
#include "ImageProc.h"

using namespace std;

namespace acllite {
ImageProc::ImageProc(int deviceId):isReleased_(false), stream_(nullptr),
    dvppChannelDesc_(nullptr), jpegeConfig_(nullptr),
    resizeConfig_(nullptr), cropArea_(nullptr), pasteArea_(nullptr), 
    srcPicDesc_(nullptr), dstPicDesc_(nullptr), dstBuffer_(nullptr)
{
    Init();
}

ImageProc::~ImageProc()
{
    DestroyResource();
}

void ImageProc::DestroyResource()
{
    if (isReleased_) {
        return;
    }
    aclError aclRet;
    if (dvppChannelDesc_ != nullptr) {
        aclRet = acldvppDestroyChannel(dvppChannelDesc_);
        CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Destroy dvpp channel failed. ERROR: %d", aclRet); return);
        (void)acldvppDestroyChannelDesc(dvppChannelDesc_);
        dvppChannelDesc_ = nullptr;
    }

    if (stream_ != nullptr) {
        aclRet = aclrtDestroyStream(stream_);
        CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Destroy dvpp stream failed. ERROR: %d", aclRet); return);
        stream_ = nullptr;
    }
    isReleased_ = true;
}

bool ImageProc::Init()
{
    aclError aclRet;
    aclRet = aclrtCreateStream(&stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Create dvpp stream failed. ERROR: %d", aclRet); return false);

    dvppChannelDesc_ = acldvppCreateChannelDesc();
    CHECK_RET(dvppChannelDesc_ != nullptr, LOG_PRINT("[ERROR] Create dvpp channel desc failed."); return false);

    // todo: ever need to set acldvppSetChannelDescMode?
    aclRet = acldvppCreateChannel(dvppChannelDesc_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Create dvpp channel failed. ERROR: %d", aclRet); return false);

    socVersion_ = aclrtGetSocName();
    nPos_ = socVersion_.npos;
    pos310P_ = socVersion_.find("Ascend310P");
    pos310B_ = socVersion_.find("Ascend310B");
    pos910B_ = socVersion_.find("Ascend910B");
    LOG_PRINT("[INFO] Init dvpp resource success.");
    return true;
}

/************************Read fun************************/
bool ImageProc::SetJpegdPicDescNV12(uint32_t srcWidth, uint32_t srcHeight, acldvppPixelFormat dstFormat)
{
    // case YUV420SP NV12 8bit / YUV420SP NV21 8bit
    dstWidth_ = ALIGN_UP2(srcWidth);
    dstHeight_ = ALIGN_UP2(srcHeight);
    if (pos310P_ != nPos_ || pos310B_ != nPos_ || pos910B_ != nPos_) {
        dstWidthStride_ = ALIGN_UP64(dstWidth_);
        dstHeightStride_ = ALIGN_UP16(dstHeight_);
    } else {
        dstWidthStride_ = ALIGN_UP128(dstWidth_);
        dstHeightStride_ = ALIGN_UP16(dstHeight_);
    }
    aclError aclRet = acldvppMalloc(&dstBuffer_, dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Malloc dvpp memory for jpegd failed. ERROR: %d", aclRet); return false);
    
    dstPicDesc_ = acldvppCreatePicDesc();
    CHECK_RET(dstPicDesc_ != nullptr, LOG_PRINT("[ERROR] Create dvpp pic desc for jpegd failed."); return false);
    acldvppSetPicDescData(dstPicDesc_, dstBuffer_);
    acldvppSetPicDescSize(dstPicDesc_, dstBufferSize_);
    acldvppSetPicDescFormat(dstPicDesc_, dstFormat);
    acldvppSetPicDescWidth(dstPicDesc_, dstWidth_);
    acldvppSetPicDescHeight(dstPicDesc_, dstHeight_);
    acldvppSetPicDescWidthStride(dstPicDesc_, dstWidthStride_);
    acldvppSetPicDescHeightStride(dstPicDesc_, dstHeightStride_);
    return true;
}

ImageData ImageProc::JpegD(void*& hostData, uint32_t size, acldvppPixelFormat dstFormat)
{
    // struct ImageData
    // get image info
    ImageData dst;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t ch = 0;
    acldvppJpegFormat srcFormat;
    aclError aclRet = acldvppJpegGetImageInfoV2(hostData, size, &width, &height, &ch, &srcFormat);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Get image info from jpeg pic failed. ERROR: %d", aclRet); return dst);
    aclRet = acldvppJpegPredictDecSize(hostData, size, dstFormat, &dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Get image size predict from jpeg pic failed. ERROR: %d", aclRet); return dst);

    // malloc device memory for jpegD input
    void* deviceMem = nullptr;
    aclRet = acldvppMalloc(&deviceMem, size);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppMalloc failed. ERROR: %d", aclRet); return dst);
    // copy to device
    aclRet = aclrtMemcpy(deviceMem, size, hostData, size, ACL_MEMCPY_HOST_TO_DEVICE);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
        acldvppFree(deviceMem);
        return dst;
    }
    delete[]((uint8_t *)hostData);
    // set jpegD output desc
    bool ret = SetJpegdPicDescNV12(width, height, dstFormat);
    CHECK_RET(ret, LOG_PRINT("[ERROR] set jpegd output desc failed. ERROR: %d", ret); return dst);
    // jpegD
    aclRet = acldvppJpegDecodeAsync(dvppChannelDesc_,
                                    reinterpret_cast<void *>(deviceMem),
                                    size, dstPicDesc_, stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppJpegDecodeAsync failed. ERROR: %d", aclRet); return dst);
    aclRet = aclrtSynchronizeStream(stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppJpegDecodeAsync sync stream failed. ERROR: %d", aclRet); return dst);
    dst.data = SHARED_PTR_DVPP_BUF(dstBuffer_);
    dst.size = dstBufferSize_;
    dst.width = dstWidth_;
    dst.height = dstHeight_;
    dst.alignWidth = dstWidthStride_;
    dst.alignHeight = dstHeightStride_;
    dst.format = dstFormat;
    // release jpegd input mem and output pic desc
    acldvppFree(deviceMem);
    if (dstPicDesc_ != nullptr) {
        acldvppDestroyPicDesc(dstPicDesc_);
        dstPicDesc_ = nullptr;
    }
    return dst;
}

bool ImageProc::SetPngdPicDescRGB(uint32_t srcWidth, uint32_t srcHeight, acldvppPixelFormat dstFormat)
{
    // case RGB888
    dstWidth_ = srcWidth;
    dstHeight_ = srcHeight;
    dstWidthStride_ = ALIGN_UP128(srcWidth);
    dstHeightStride_ = ALIGN_UP16(srcHeight);
    aclError aclRet = acldvppMalloc(&dstBuffer_, dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Malloc dvpp memory for pngd failed. ERROR: %d", aclRet); return false);
    
    dstPicDesc_ = acldvppCreatePicDesc();
    CHECK_RET(dstPicDesc_ != nullptr, LOG_PRINT("[ERROR] Create dvpp pic desc for pngd failed."); return false);
    acldvppSetPicDescData(dstPicDesc_, dstBuffer_);
    acldvppSetPicDescSize(dstPicDesc_, dstBufferSize_);
    acldvppSetPicDescFormat(dstPicDesc_, dstFormat);
    acldvppSetPicDescWidth(dstPicDesc_, dstWidth_);
    acldvppSetPicDescHeight(dstPicDesc_, dstHeight_);
    acldvppSetPicDescWidthStride(dstPicDesc_, dstWidthStride_);
    acldvppSetPicDescHeightStride(dstPicDesc_, dstHeightStride_);
    return true;
}

ImageData ImageProc::PngD(void*& hostData, uint32_t size, acldvppPixelFormat dstFormat)
{
    // struct ImageData
    // get image info
    ImageData dst;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t ch = 0;
    aclError aclRet = acldvppPngGetImageInfo(hostData, size, &width, &height, &ch);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Get image info from png pic failed. ERROR: %d", aclRet); return dst);
    aclRet = acldvppPngPredictDecSize(hostData, size, dstFormat, &dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Get image size predict from png pic failed. ERROR: %d", aclRet); return dst);

    // malloc device memory for pngD input
    void* deviceMem = nullptr;
    aclRet = acldvppMalloc(&deviceMem, size);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppMalloc failed. ERROR: %d", aclRet); return dst);
    // copy to device
    aclRet = aclrtMemcpy(deviceMem, size, hostData, size, ACL_MEMCPY_HOST_TO_DEVICE);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
        acldvppFree(deviceMem);
        return dst;
    }
    delete[]((uint8_t *)hostData);
    // set pngD output desc
    bool ret = SetPngdPicDescRGB(width, height, dstFormat);
    CHECK_RET(ret, LOG_PRINT("[ERROR] set pngd output desc failed. ERROR: %d", ret); return dst);
    // pngD
    aclRet = acldvppPngDecodeAsync(dvppChannelDesc_,
                                    reinterpret_cast<void *>(deviceMem),
                                    size, dstPicDesc_, stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppPngDecodeAsync failed. ERROR: %d", aclRet); return dst);
    aclRet = aclrtSynchronizeStream(stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppPngDecodeAsync sync stream failed. ERROR: %d", aclRet); return dst);
    dst.data = SHARED_PTR_DVPP_BUF(dstBuffer_);
    dst.size = dstBufferSize_;
    dst.width = dstWidth_;
    dst.height = dstHeight_;
    dst.alignWidth = dstWidthStride_;
    dst.alignHeight = dstHeightStride_;
    dst.format = dstFormat;
    // release pngd input mem and output pic desc
    acldvppFree(deviceMem);
    if (dstPicDesc_ != nullptr) {
        acldvppDestroyPicDesc(dstPicDesc_);
        dstPicDesc_ = nullptr;
    }
    return dst;
}

ImageData ImageProc::Read(const string& filePath, acldvppPixelFormat imgFormat)
{
    // read file to host 
    void* hostDataBuf = nullptr;
    uint32_t hostDataSize = 0;
    ReadBinFile(filePath, hostDataBuf, hostDataSize);

    int splitPos = filePath.find_last_of('/');
    string fileName = filePath.substr(splitPos + 1);
    // get file type
    int pos = fileName.find('.');
    string fileType = fileName.substr(pos + 1);
    if (fileType=="jpg" || fileType=="jpeg" || fileType=="JPG" || fileType=="JPEG") {
        return JpegD(hostDataBuf, hostDataSize, imgFormat);
    } else if (fileType=="png") {
        return PngD(hostDataBuf, hostDataSize, imgFormat);
    } else {
        LOG_PRINT("[ERROR] Read file type not supported.");
        ImageData dst;
        return dst;
    }
}

/************************resize fun************************/
bool ImageProc::SetVpcInputPicDescYUV422P(ImageData& src)
{
    // case YUV422Packed 8bit
    srcWidth_ = ALIGN_UP2(src.width);
    srcHeight_ = src.height;
    if (pos310B_ != nPos_ || pos910B_ != nPos_) {
        if (src.alignWidth == 0 || src.alignHeight == 0) {
            srcWidthStride_ = srcWidth_ * 2;
            srcHeightStride_ = srcHeight_;
        } else {
            srcWidthStride_ = src.alignWidth;
            srcHeightStride_ = src.alignHeight;
        }
    } else if (pos310P_ != nPos_) {
        if (src.alignWidth == 0 || src.alignHeight == 0) {
            srcWidthStride_ = ALIGN_UP16(srcWidth_) * 2;
            srcHeightStride_ = srcHeight_;
        } else {
            srcWidthStride_ = src.alignWidth;
            srcHeightStride_ = src.alignHeight;
        }
    } else {
        if (src.alignWidth == 0 || src.alignHeight == 0) {
            srcWidthStride_ = ALIGN_UP16(srcWidth_) * 2;
            srcHeightStride_ = ALIGN_UP2(srcHeight_);
        } else {
            srcWidthStride_ = src.alignWidth;
            srcHeightStride_ = src.alignHeight;
        }
    }
    
    srcBufferSize_ = YUV422P_SIZE(srcWidthStride_, srcHeightStride_);
    srcPicDesc_ = acldvppCreatePicDesc();
    CHECK_RET(srcPicDesc_ != nullptr, LOG_PRINT("[ERROR] Create dvpp pic desc for resize input failed."); return false);
    acldvppSetPicDescData(srcPicDesc_, src.data.get());
    acldvppSetPicDescSize(srcPicDesc_, srcBufferSize_);
    acldvppSetPicDescFormat(srcPicDesc_, src.format);
    acldvppSetPicDescWidth(srcPicDesc_, srcWidth_);
    acldvppSetPicDescHeight(srcPicDesc_, srcHeight_);
    acldvppSetPicDescWidthStride(srcPicDesc_, srcWidthStride_);
    acldvppSetPicDescHeightStride(srcPicDesc_, srcHeightStride_);
    return true;
}

bool ImageProc::SetVpcInputPicDescYUV420SP(ImageData& src)
{
    // case YUV420SP NV12 8bit / YUV420SP NV21 8bit
    srcWidth_ = ALIGN_UP2(src.width);
    srcHeight_ = ALIGN_UP2(src.height);
    if (pos310B_ != nPos_ || pos910B_ != nPos_) {
        if (src.alignWidth == 0 || src.alignHeight == 0) {
            srcWidthStride_ = srcWidth_;
            srcHeightStride_ = ALIGN_UP2(srcHeight_);
        } else {
            srcWidthStride_ = src.alignWidth;
            srcHeightStride_ = src.alignHeight;
        }
    } else {
        if (src.alignWidth == 0 || src.alignHeight == 0) {
            srcWidthStride_ = ALIGN_UP16(srcWidth_);
            srcHeightStride_ = ALIGN_UP2(srcHeight_);
        } else {
            srcWidthStride_ = src.alignWidth;
            srcHeightStride_ = src.alignHeight;
        }
    }
    srcBufferSize_ = YUV420SP_SIZE(srcWidthStride_, srcHeightStride_);
    srcPicDesc_ = acldvppCreatePicDesc();
    CHECK_RET(srcPicDesc_ != nullptr, LOG_PRINT("[ERROR] Create dvpp pic desc for resize input failed."); return false);
    acldvppSetPicDescData(srcPicDesc_, src.data.get());
    acldvppSetPicDescSize(srcPicDesc_, srcBufferSize_);
    acldvppSetPicDescFormat(srcPicDesc_, src.format);
    acldvppSetPicDescWidth(srcPicDesc_, srcWidth_);
    acldvppSetPicDescHeight(srcPicDesc_, srcHeight_);
    acldvppSetPicDescWidthStride(srcPicDesc_, srcWidthStride_);
    acldvppSetPicDescHeightStride(srcPicDesc_, srcHeightStride_);
    return true;
}

bool ImageProc::SetVpcOutputPicDescYUV420SP(ImageSize dsize, acldvppPixelFormat format)
{
    // case YUV420SP NV12 8bit / YUV420SP NV21 8bit
    dstWidth_ = ALIGN_UP2(dsize.width);
    dstHeight_ = ALIGN_UP2(dsize.height);
    if (pos310B_ != nPos_ || pos910B_ != nPos_) {
        dstWidthStride_ = dstWidth_;
        dstHeightStride_ = ALIGN_UP2(dstHeight_);
    } else {
        dstWidthStride_ = ALIGN_UP16(dstWidth_);
        dstHeightStride_ = ALIGN_UP2(dstHeight_);
    }
    dstBufferSize_ = YUV420SP_SIZE(dstWidthStride_, dstHeightStride_);
    aclError aclRet = acldvppMalloc(&dstBuffer_, dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Malloc dvpp memory for resize output failed. ERROR: %d", aclRet); return false);
    
    dstPicDesc_ = acldvppCreatePicDesc();
    CHECK_RET(dstPicDesc_ != nullptr, LOG_PRINT("[ERROR] Create dvpp pic desc for resize output failed."); return false);
    acldvppSetPicDescData(dstPicDesc_, dstBuffer_);
    acldvppSetPicDescSize(dstPicDesc_, dstBufferSize_);
    acldvppSetPicDescFormat(dstPicDesc_, format);
    acldvppSetPicDescWidth(dstPicDesc_, dstWidth_);
    acldvppSetPicDescHeight(dstPicDesc_, dstHeight_);
    acldvppSetPicDescWidthStride(dstPicDesc_, dstWidthStride_);
    acldvppSetPicDescHeightStride(dstPicDesc_, dstHeightStride_);
    return true;
}

void ImageProc::ResizeCommon(ImageData& src, ImageData& dst, ImageSize dsize)
{
    acldvppPixelFormat format;
    switch (src.format) {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        case PIXEL_FORMAT_YUYV_PACKED_422:
            format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            SetVpcInputPicDescYUV422P(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        default:
            LOG_PRINT("[ERROR] input format not supported.");
            return;
    }
    // config
    resizeConfig_ = acldvppCreateResizeConfig();
    CHECK_RET(resizeConfig_ != nullptr, LOG_PRINT("[ERROR] Dvpp resize failed for create config failed."); return);
    // resize pic
    aclError aclRet = acldvppVpcResizeAsync(dvppChannelDesc_, srcPicDesc_,
                                            dstPicDesc_, resizeConfig_, stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppVpcResizeAsync failed. ERROR: %d", aclRet); return);
    aclRet = aclrtSynchronizeStream(stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] resize aclrtSynchronizeStream failed. ERROR: %d", aclRet); return);

    dst.data = SHARED_PTR_DVPP_BUF(dstBuffer_);
    dst.size = dstBufferSize_;
    dst.width = dstWidth_;
    dst.height = dstHeight_;
    dst.alignWidth = dstWidthStride_;
    dst.alignHeight = dstHeightStride_;
    dst.format = format;

    if (resizeConfig_ != nullptr) {
        (void)acldvppDestroyResizeConfig(resizeConfig_);
        resizeConfig_ = nullptr;
    }
    if (srcPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(srcPicDesc_);
        srcPicDesc_ = nullptr;
    }
    if (dstPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(dstPicDesc_);
        dstPicDesc_ = nullptr;
    }
    return;
}

void ImageProc::ResizeProportionalUpperLeft(ImageData& src, ImageData& dst, ImageSize dsize)
{
    acldvppPixelFormat format;
    switch (src.format) {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        case PIXEL_FORMAT_YUYV_PACKED_422:
            format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            SetVpcInputPicDescYUV422P(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        default:
            LOG_PRINT("[ERROR] input format not supported.");
            return;
    }
    // crop area
    // must even
    uint32_t cropLeftOffset = 0;
    // must even
    uint32_t cropTopOffset = 0;
    // must odd
    uint32_t cropRightOffset = (((cropLeftOffset + src.width) >> 1) << 1) -1;
    // must odd
    uint32_t cropBottomOffset = (((cropTopOffset + src.height) >> 1) << 1) -1;
    // data created to describe area location
    cropArea_ = acldvppCreateRoiConfig(cropLeftOffset, cropRightOffset,
                                       cropTopOffset, cropBottomOffset);
    CHECK_RET(cropArea_ != nullptr, LOG_PRINT("[ERROR] acldvppCreateRoiConfig for crop config failed."); return);
 
    //paste area
    float rx = (float)src.width / (float)dsize.width;
    float ry = (float)src.height / (float)dsize.height;
    int dx = 0;
    int dy = 0;
    float r = 0.0f;
    if (rx > ry) {
        dx = 0;
        r = rx;
        dy = (dsize.height - src.height / r) / 2;
    } else {
        dy = 0;
        r = ry;
        dx = (dsize.width - src.width / r) / 2;
    }

    // must even
    uint32_t pasteLeftOffset = 0;
    // must even
    uint32_t pasteTopOffset = 0;
    // must odd
    uint32_t pasteRightOffset = (((dsize.width - 2 * dx) >> 1) << 1) -1;;
    // must odd
    uint32_t pasteBottomOffset = (((dsize.height -  2 * dy) >> 1) << 1) -1;

    pasteArea_ = acldvppCreateRoiConfig(pasteLeftOffset, pasteRightOffset,
                                        pasteTopOffset, pasteBottomOffset);
    CHECK_RET(pasteArea_ != nullptr, LOG_PRINT("[ERROR] acldvppCreateRoiConfig for paste config failed."); return);

    // crop and patse pic
    aclError aclRet = acldvppVpcCropAndPasteAsync(dvppChannelDesc_, srcPicDesc_,
                                                  dstPicDesc_, cropArea_,
                                                  pasteArea_, stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppVpcCropAndPasteAsync failed. ERROR: %d", aclRet); return);

    aclRet = aclrtSynchronizeStream(stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] crop and paste aclrtSynchronizeStream failed. ERROR: %d", aclRet); return);

    dst.data = SHARED_PTR_DVPP_BUF(dstBuffer_);
    dst.size = dstBufferSize_;
    dst.width = dstWidth_;
    dst.height = dstHeight_;
    dst.alignWidth = dstWidthStride_;
    dst.alignHeight = dstHeightStride_;
    dst.format = format;
    if (cropArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(cropArea_);
        cropArea_ = nullptr;
    }
    if (pasteArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(pasteArea_);
        pasteArea_ = nullptr;
    }
    if (srcPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(srcPicDesc_);
        srcPicDesc_ = nullptr;
    }
    if (dstPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(dstPicDesc_);
        dstPicDesc_ = nullptr;
    }
    return;
}

void ImageProc::ResizeProportionalCenter(ImageData& src, ImageData& dst, ImageSize dsize)
{
    acldvppPixelFormat format;
    switch (src.format) {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        case PIXEL_FORMAT_YUYV_PACKED_422:
            format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            SetVpcInputPicDescYUV422P(src);
            SetVpcOutputPicDescYUV420SP(dsize, format);
            break;
        default:
            LOG_PRINT("[ERROR] input format not supported.");
            return;
    }
    // crop area
    // must even
    uint32_t cropLeftOffset = 0;
    // must even
    uint32_t cropTopOffset = 0;
    // must odd
    uint32_t cropRightOffset = (((cropLeftOffset + src.width) >> 1) << 1) -1;
    // must odd
    uint32_t cropBottomOffset = (((cropTopOffset + src.height) >> 1) << 1) -1;
    // data created to describe area location
    cropArea_ = acldvppCreateRoiConfig(cropLeftOffset, cropRightOffset,
                                       cropTopOffset, cropBottomOffset);
    CHECK_RET(cropArea_ != nullptr, LOG_PRINT("[ERROR] acldvppCreateRoiConfig for crop config failed."); return);
    //paste area
    float rx = (float)src.width / (float)dsize.width;
    float ry = (float)src.height / (float)dsize.height;
    int dx = 0;
    int dy = 0;
    float r = 0.0f;
    if (rx > ry) {
        dx = 0;
        r = rx;
        dy = (dsize.height - src.height / r) / 2;
    } else {
        dy = 0;
        r = ry;
        dx = (dsize.width - src.width / r) / 2;
    }
    // must even
    uint32_t pasteLeftOffset = ALIGN_UP16(dx);
    // must even
    uint32_t pasteTopOffset = ALIGN_UP2(dy);
    // must odd
    uint32_t pasteRightOffset = (((dsize.width + pasteLeftOffset - 2 * dx) >> 1) << 1) -1;
    // must odd
    uint32_t pasteBottomOffset = (((dsize.height - dy) >> 1) << 1) -1;  

    pasteArea_ = acldvppCreateRoiConfig(pasteLeftOffset, pasteRightOffset,
                                        pasteTopOffset, pasteBottomOffset);
    CHECK_RET(pasteArea_ != nullptr, LOG_PRINT("[ERROR] acldvppCreateRoiConfig for paste config failed."); return);

    // crop and patse pic
    aclError aclRet = acldvppVpcCropAndPasteAsync(dvppChannelDesc_, srcPicDesc_,
                                                  dstPicDesc_, cropArea_,
                                                  pasteArea_, stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppVpcCropAndPasteAsync failed. ERROR: %d", aclRet); return);

    aclRet = aclrtSynchronizeStream(stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] crop and paste aclrtSynchronizeStream failed. ERROR: %d", aclRet); return);

    dst.data = SHARED_PTR_DVPP_BUF(dstBuffer_);
    dst.size = dstBufferSize_;
    dst.width = dstWidth_;
    dst.height = dstHeight_;
    dst.alignWidth = dstWidthStride_;
    dst.alignHeight = dstHeightStride_;
    dst.format = format;
    if (cropArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(cropArea_);
        cropArea_ = nullptr;
    }
    if (pasteArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(pasteArea_);
        pasteArea_ = nullptr;
    }
    if (srcPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(srcPicDesc_);
        srcPicDesc_ = nullptr;
    }
    if (dstPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(dstPicDesc_);
        dstPicDesc_ = nullptr;
    }
    return;
}

void ImageProc::Resize(ImageData& src, ImageData& dst, ImageSize dSize, ResizeType type)
{
    if (dSize.width<=0 || dSize.height<=0) {
        LOG_PRINT("[ERROR] dsize(%d, %d) not supported", dSize.width, dSize.height);
    }
    switch (type) {
        case RESIZE_COMMON:
            ResizeCommon(src, dst, dSize);
            break;
        case RESIZE_PROPORTIONAL_UPPER_LEFT:
            ResizeProportionalUpperLeft(src, dst, dSize);
            break;
        case RESIZE_PROPORTIONAL_CENTER:
            ResizeProportionalCenter(src, dst, dSize);
            break;
        default:
            LOG_PRINT("[ERROR] resize type not supported.");
            break;
    }
    return;
}

void ImageProc::Crop(ImageData& src, ImageData& dst, DvRect dRect, ImageSize dSize)
{
    if (dRect.lx<0 || dRect.ly<0 || dRect.rx>src.width || dRect.ry>src.height) {
        LOG_PRINT("[ERROR] dRect(%d, %d, %d, %d) not supported", dRect.lx, dRect.ly, dRect.rx, dRect.ry);
        return;
    }
    if (dSize.width<=0 || dSize.height<=0) {
        dSize.width = dRect.rx - dRect.lx;
        dSize.height = dRect.ry - dRect.ly;
    }
    acldvppPixelFormat format;
    switch (src.format) {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dSize, format);
            break;
        case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
            format = src.format;
            SetVpcInputPicDescYUV420SP(src);
            SetVpcOutputPicDescYUV420SP(dSize, format);
            break;
        case PIXEL_FORMAT_YUYV_PACKED_422:
            format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            SetVpcInputPicDescYUV422P(src);
            SetVpcOutputPicDescYUV420SP(dSize, format);
            break;
        default:
            LOG_PRINT("[ERROR] input format not supported.");
            return;
    }
    // must even
    uint32_t cropLeftOffset = (dRect.lx >> 1) << 1;
    // must even
    uint32_t cropTopOffset = (dRect.ly >> 1) << 1;
    // must odd
    uint32_t cropRightOffset = ((dRect.rx >> 1) << 1) - 1;
    // must odd
    uint32_t cropBottomOffset = ((dRect.ry >> 1) << 1) - 1;

    cropArea_ = acldvppCreateRoiConfig(cropLeftOffset, cropRightOffset,
                                       cropTopOffset, cropBottomOffset);
    CHECK_RET(cropArea_ != nullptr, LOG_PRINT("[ERROR] acldvppCreateRoiConfig for crop config failed."); return);

    // must even
    uint32_t pasteLeftOffset = 0;
    // must even
    uint32_t pasteTopOffset = 0;
    // must odd
    uint32_t pasteRightOffset = (((pasteLeftOffset + dSize.width) >> 1) << 1) -1;
    // must odd
    uint32_t pasteBottomOffset = (((pasteTopOffset + dSize.height) >> 1) << 1) -1;

    pasteArea_ = acldvppCreateRoiConfig(pasteLeftOffset, pasteRightOffset,
                                        pasteTopOffset, pasteBottomOffset);
    CHECK_RET(pasteArea_ != nullptr, LOG_PRINT("[ERROR] acldvppCreateRoiConfig for paste config failed."); return);

    // crop and patse pic
    aclError aclRet = acldvppVpcCropAndPasteAsync(dvppChannelDesc_, srcPicDesc_,
                                                  dstPicDesc_, cropArea_,
                                                  pasteArea_, stream_);
    
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppVpcCropAndPasteAsync failed. ERROR: %d", aclRet); return);

    aclRet = aclrtSynchronizeStream(stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] crop aclrtSynchronizeStream failed. ERROR: %d", aclRet); return);

    dst.format = format;
    dst.width = dSize.width;
    dst.height = dSize.height;
    dst.alignWidth = dstWidthStride_;
    dst.alignHeight = dstHeightStride_;
    dst.size = dstBufferSize_;
    dst.data = SHARED_PTR_DVPP_BUF(dstBuffer_);
    if (cropArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(cropArea_);
        cropArea_ = nullptr;
    }
    if (pasteArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(pasteArea_);
        pasteArea_ = nullptr;
    }
    if (srcPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(srcPicDesc_);
        srcPicDesc_ = nullptr;
    }
    if (dstPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(dstPicDesc_);
        dstPicDesc_ = nullptr;
    }
    return;
}

/************************Write fun************************/
bool ImageProc::SetJpgePicDescNV12(ImageData& src)
{
    // case YUV420SP NV12 8bit / YUV420SP NV21 8bit
    srcWidth_ = ALIGN_UP2(src.width);
    srcHeight_ = ALIGN_UP2(src.height);

    ImageData jpgeInputPic;
    jpgeInputPic.width = srcWidth_;
    jpgeInputPic.height = srcHeight_;
    jpgeInputPic.alignWidth = ALIGN_UP16(src.width);
    jpgeInputPic.alignHeight = src.alignHeight;
    jpgeInputPic.size = YUV420SP_SIZE(jpgeInputPic.alignWidth, jpgeInputPic.alignHeight);
    void* srcBuffer_ = nullptr;
    aclError aclRet = acldvppMalloc(&srcBuffer_, jpgeInputPic.size);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Malloc dvpp memory for jpege output failed. ERROR: %d", aclRet); return false);
    jpgeInputPic.data = SHARED_PTR_DVPP_BUF(srcBuffer_);

    bool ret = AlignSpImage(jpgeInputPic, jpgeInputPic.data.get(), src.data.get());
    CHECK_RET(ret, LOG_PRINT("[ERROR] AlignSpImage for jpege failed. ERROR: %d", aclRet); return false);
    
    srcPicDesc_ = acldvppCreatePicDesc();
    CHECK_RET(srcPicDesc_ != nullptr, LOG_PRINT("[ERROR] Create dvpp pic desc for jpege output failed."); return false);
    acldvppSetPicDescData(srcPicDesc_, reinterpret_cast<void *>(jpgeInputPic.data.get()));
    acldvppSetPicDescSize(srcPicDesc_, jpgeInputPic.size);
    acldvppSetPicDescFormat(srcPicDesc_, src.format);
    acldvppSetPicDescWidth(srcPicDesc_, jpgeInputPic.width);
    acldvppSetPicDescHeight(srcPicDesc_, jpgeInputPic.height);
    acldvppSetPicDescWidthStride(srcPicDesc_, jpgeInputPic.alignWidth);
    acldvppSetPicDescHeightStride(srcPicDesc_, jpgeInputPic.alignHeight);
    return true;
}

bool ImageProc::AlignSpImage(const ImageData &picDesc, uint8_t *inBuffer, uint8_t *imageBuffer)
{
    aclError aclRet;
    // copy y data
    for (auto i = 0; i < picDesc.height; i++) {
        aclRet = aclrtMemcpy(inBuffer + i * picDesc.alignWidth, picDesc.width,
            imageBuffer + i * picDesc.width, picDesc.width, ACL_MEMCPY_DEVICE_TO_DEVICE);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] memcpy failed, errorCode = %d", static_cast<int32_t>(aclRet));
            return false;
        }
    }
    // copy uv data
    int32_t uvHigh = picDesc.height;
    int32_t uvWidthCoefficient = 1;
    switch (picDesc.format) {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
            uvHigh /= 2;
            break;
        default:
            LOG_PRINT("[ERROR] format(%d) cannot support", picDesc.format);
            return false;
    }
    uint8_t *uvSrcData = inBuffer + picDesc.alignWidth * picDesc.alignHeight;
    uint8_t *uvDstData = imageBuffer + picDesc.width * picDesc.height;
    for (auto i = 0; i < uvHigh; i++) {
        aclRet = aclrtMemcpy(uvSrcData + i * picDesc.alignWidth * uvWidthCoefficient, picDesc.width * uvWidthCoefficient,
            uvDstData + i * picDesc.width * uvWidthCoefficient, picDesc.width * uvWidthCoefficient, ACL_MEMCPY_DEVICE_TO_DEVICE);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] memcpy failed, errorCode = %d", static_cast<int32_t>(aclRet));
            return false;
        }
    }

    return true;
}

bool ImageProc::Write(const string& fileName, ImageData& img)
{
    bool ret = SetJpgePicDescNV12(img);
    CHECK_RET(ret, LOG_PRINT("[ERROR] SetJpgePicDescNV12 failed."); return false);

    uint32_t encodeLevel = 100; // default optimal level (0-100)
    jpegeConfig_ = acldvppCreateJpegeConfig();
    aclError aclRet = acldvppSetJpegeConfigLevel(jpegeConfig_, encodeLevel);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppSetJpegeConfigLevel failed. ERROR: %d", aclRet); return false);

    aclRet = acldvppJpegPredictEncSize(srcPicDesc_, jpegeConfig_, &dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppJpegPredictEncSize failed. ERROR: %d", aclRet); return false);
    aclRet = acldvppMalloc(&dstBuffer_, dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Malloc dvpp memory for jpege output failed. ERROR: %d", aclRet); return false);
    
    aclRet = acldvppJpegEncodeAsync(dvppChannelDesc_, srcPicDesc_, dstBuffer_,
                                    &dstBufferSize_, jpegeConfig_, stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] acldvppJpegEncodeAsync failed. ERROR: %d", aclRet); return false);
    aclRet = aclrtSynchronizeStream(stream_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] Dvpp jpege sync stream  failed. ERROR: %d", aclRet); return false);
    // malloc host memory for jpegE output
    void* hostData = nullptr;
    aclRet = aclrtMallocHost(&hostData, dstBufferSize_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtMallocHost failed. ERROR: %d", aclRet); return false);
    // copy to host
    aclRet = aclrtMemcpy(hostData, dstBufferSize_, dstBuffer_, dstBufferSize_, ACL_MEMCPY_DEVICE_TO_HOST);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
        aclrtFreeHost(hostData);
        return false;
    }
    acldvppFree(dstBuffer_);
    SaveBinFile(fileName, hostData, dstBufferSize_);
    aclrtFreeHost(hostData);
    if (jpegeConfig_ != nullptr) {
        (void)acldvppDestroyJpegeConfig(jpegeConfig_);
        jpegeConfig_ = nullptr;
    }

    if (srcPicDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(srcPicDesc_);
        srcPicDesc_ = nullptr;
    }
    return true;
}
}  // namespace acllite