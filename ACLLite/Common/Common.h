
#ifndef COMMON_H
#define COMMON_H
#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <math.h> 
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdint>
#include <regex>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <thread>
#include <vector>
#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
  
#define ALIGN_UP(num, align) (((num) + (align) - 1) & ~((align) - 1))
#define ALIGN_UP2(num) ALIGN_UP(num, 2)
#define ALIGN_UP16(num) ALIGN_UP(num, 16)
#define ALIGN_UP64(num) ALIGN_UP(num, 64)
#define ALIGN_UP128(num) ALIGN_UP(num, 128)

#define YUV420SP_SIZE(width, height) ((width) * (height) * 3 / 2)
#define YUV422P_SIZE(width, height) ((width) * (height))

#define SHARED_PTR_U8_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { delete[](p); }))
#define SHARED_PTR_HOST_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { aclrtFreeHost(p); }))
#define SHARED_PTR_DEV_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { aclrtFree(p); }))
#define SHARED_PTR_DVPP_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { acldvppFree(p); }))

#define CHECK_RET(cond, return_expr) \
  do {                               \
    if (!(cond)) {                   \
      return_expr;                   \
    }                                \
  } while (0)

#define LOG_PRINT(message, ...)     \
  do {                              \
    printf(message"\n", ##__VA_ARGS__); \
  } while (0)

namespace acllite {
struct DataInfo {
    void* data;
    uint32_t size;
};

struct InferenceOutput {
    std::shared_ptr<void> data = nullptr;
    uint32_t size;
};

enum memoryLocation {
    MEMORY_NORMAL = 0,
    MEMORY_HOST,
    MEMORY_DEVICE,
    MEMORY_DVPP,
    MEMORY_INVALID_TYPE
};

enum ResizeType {
    RESIZE_COMMON = 0,
    RESIZE_PROPORTIONAL_UPPER_LEFT = 1,
    RESIZE_PROPORTIONAL_CENTER = 2,
};

enum StreamProperty {
    FRAME_WIDTH = 1,
    FRAME_HEIGHT = 2,
    VIDEO_FPS = 3,
    OUTPUT_IMAGE_FORMAT = 4,
    RTSP_TRANSPORT = 5,
    STREAM_FORMAT = 6
};

struct ImageSize {
    uint32_t width = 0;
    uint32_t height = 0;
    ImageSize() {}
    ImageSize(uint32_t x, uint32_t y) : width(x), height(y) {}
};

struct DvRect {
    uint32_t lx;
    uint32_t ly;
    uint32_t rx;
    uint32_t ry;
    DvRect():lx(0), ly(0), rx(0), ry(0) {}
    DvRect(uint32_t lxv, uint32_t lyv,
        uint32_t rxv, uint32_t ryv) : lx(lxv),
        ly(lyv), rx(rxv), ry(ryv) {}
};

struct ImageData {
    std::shared_ptr<uint8_t> data = nullptr;
    uint32_t size = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t alignWidth = 0;
    uint32_t alignHeight = 0;
    acldvppPixelFormat format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    ImageData() {}
    ImageData(std::shared_ptr<uint8_t> buf, uint32_t bufSize,
        uint32_t x, uint32_t y, acldvppPixelFormat fmt) : data(buf), size(bufSize),
        width(x), height(y), format(fmt) {}
};

struct FrameData {
    bool isFinished = false;
    uint32_t frameId = 0;
    uint32_t size = 0;
    void* data = nullptr;
};

enum VencStatus {
    STATUS_VENC_INIT = 0,
    STATUS_VENC_WORK,
    STATUS_VENC_FINISH,
    STATUS_VENC_EXIT,
    STATUS_VENC_ERROR
};

enum StreamType {
    STREAM_VIDEO = 0,
    STREAM_RTSP,
};

enum DecodeStatus {
    DECODE_ERROR  = -1,
    DECODE_UNINIT = 0,
    DECODE_READY  = 1,
    DECODE_START  = 2,
    DECODE_FFMPEG_FINISHED = 3,
    DECODE_DVPP_FINISHED = 4,
    DECODE_FINISHED = 5
};

typedef int Result;
const int RES_OK = 0;
const int RES_ERROR = 1;

void SaveBinFile(const std::string& filename, const void* data, uint32_t size);
bool ReadBinFile(const std::string& fileName, void*& data, uint32_t& size);
void* CopyDataToDevice(void* data, uint32_t size);
aclrtContext GetContextByDevice(int32_t devId);
bool SetCurContext(aclrtContext context);

class AclLiteResource {
public:
    AclLiteResource(int32_t devId = 0);
    ~AclLiteResource();
    bool Init();
    aclrtContext GetContext()
    {
        return context_;
    }
    void Release();
private:
    bool isReleased_;
    int32_t deviceId_;
    aclrtContext context_;
};

}  // namespace acllite
#endif