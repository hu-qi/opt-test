#ifndef CAMERA_READ_H
#define CAMERA_READ_H
#include "acllite_common/Common.h"
#include "acllite_common/ThreadSafeQueue.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace acllite {
class CameraRead {
public:
    CameraRead(const std::string& videoName, int32_t deviceId = 0, aclrtContext context = nullptr);
    ~CameraRead();
    bool IsOpened();
    uint32_t Get(StreamProperty propId);
    bool Read(ImageData& frame);
    void Release();

private:
    Result Open();
    void DestroyResource();
    void StartFrameDecoder();
    bool SendFrame(void* frameData, int frameSize);
    static void DecodeFrameThread(void* decoderSelf);

private:
    int32_t deviceId_;
    aclrtContext context_;
    std::thread decodeThread_;
    std::string streamName_;
    AVFormatContext* formatContext_;
    bool isReleased_;
    bool isOpened_;
    bool isStop_;
    bool isStarted_;
    uint32_t width_;
    uint32_t height_;
    uint32_t fps_;
    ThreadSafeQueue<std::shared_ptr<ImageData>> frameQueue_;
};
}  // namespace acllite
#endif /* CAMERA_READ_H */
