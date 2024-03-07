#include "Common.h"
using namespace std;
namespace acllite {

void SaveBinFile(const string& filename, const void* data, uint32_t size)
{
    FILE *outFileFp = fopen(filename.c_str(), "wb+");
    CHECK_RET(outFileFp != nullptr, LOG_PRINT("[ERROR] Save file %s failed for open error", filename.c_str()); return);
    fwrite(data, 1, size, outFileFp);
    fflush(outFileFp);
    fclose(outFileFp);
}

bool ReadBinFile(const string& fileName, void*& data, uint32_t& size)
{
    struct stat sBuf;
    int fileStatus = stat(fileName.data(), &sBuf);
    CHECK_RET(fileStatus == 0, LOG_PRINT("[ERROR] Failed to get input file."); return false);
    CHECK_RET(S_ISREG(sBuf.st_mode) != 0, LOG_PRINT("[ERROR] %s is not a file, please enter a file.", fileName.c_str()); return false);

    std::ifstream binFile(fileName, std::ifstream::binary);
    CHECK_RET(binFile.is_open() == true, LOG_PRINT("[ERROR] Open file %s failed", fileName.c_str()); return false);

    binFile.seekg(0, binFile.end);
    uint32_t binFileBufferLen = binFile.tellg();
    CHECK_RET(binFileBufferLen != 0, LOG_PRINT("[ERROR] Binfile is empty, filename is %s", fileName.c_str()); return false);

    binFile.seekg(0, binFile.beg);
    uint8_t* binFileBufferData = new(std::nothrow) uint8_t[binFileBufferLen];
    CHECK_RET(binFileBufferData != nullptr, LOG_PRINT("[ERROR] Malloc binFileBufferData failed"); return false);

    binFile.read((char *)binFileBufferData, binFileBufferLen);
    binFile.close();

    data = binFileBufferData;
    size = binFileBufferLen;
    return true;
}

void* CopyDataToDevice(void* data, uint32_t size) {
    void* devicePtr = nullptr;
    aclError aclRet = aclrtMalloc(&devicePtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtMalloc failed. ERROR: %d", aclRet); return nullptr);
    aclrtRunMode runMode;
    aclrtGetRunMode(&runMode);
    if (runMode == ACL_HOST) {
        aclRet = aclrtMemcpy(devicePtr, size, data, size, ACL_MEMCPY_HOST_TO_DEVICE);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
            aclrtFree(devicePtr);
            devicePtr = nullptr;
            return devicePtr;
        }
    } else {
        aclRet = aclrtMemcpy(devicePtr, size, data, size, ACL_MEMCPY_DEVICE_TO_DEVICE);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
            aclrtFree(devicePtr);
            devicePtr = nullptr;
            return devicePtr;
        }
    }
    return devicePtr;
}

void* CopyDataToHost(void* data, uint32_t size) {
    void* hostPtr = nullptr;
    aclError aclRet = aclrtMallocHost(&hostPtr, size);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtMalloc failed. ERROR: %d", aclRet); return nullptr);
    aclrtRunMode runMode;
    aclrtGetRunMode(&runMode);
    if (runMode == ACL_HOST) {
        aclRet = aclrtMemcpy(hostPtr, size, data, size, ACL_MEMCPY_DEVICE_TO_HOST);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
            aclrtFreeHost(hostPtr);
            hostPtr = nullptr;
            return hostPtr;
        }
    } else {
        aclRet = aclrtMemcpy(hostPtr, size, data, size, ACL_MEMCPY_DEVICE_TO_HOST);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
            aclrtFreeHost(hostPtr);
            hostPtr = nullptr;
            return hostPtr;
        }
    }
    return hostPtr;
}

AclLiteResource::AclLiteResource(int32_t devId):
    isReleased_(false), deviceId_(devId), context_(nullptr)
{
}

AclLiteResource::~AclLiteResource()
{
    Release();
}

bool AclLiteResource::Init() {
    aclError aclRet = aclInit(nullptr);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclInit failed. ERROR: %d", aclRet); return false);
    aclRet = aclrtSetDevice(deviceId_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtSetDevice %d failed. ERROR: %d", deviceId_, aclRet); return false);
    aclRet = aclrtCreateContext(&context_, deviceId_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtCreateContext failed. ERROR: %d", aclRet); return false);
    LOG_PRINT("[INFO] InitACLResource success.");
    return true;
}

void AclLiteResource::Release()
{
    if (isReleased_) {
        return;
    }
    aclError aclRet = aclrtDestroyContext(context_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtDestroyContext failed. ERROR: %d", aclRet); return);
    aclRet = aclrtResetDevice(deviceId_);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtResetDevice %d failed. ERROR: %d", deviceId_, aclRet); return);
    aclRet = aclFinalize();
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclFinalize failed. ERROR: %d", aclRet); return);
    isReleased_ = true;
    return;
}

aclrtContext GetContextByDevice(int32_t deviceId)
{
    aclrtContext context = nullptr;
    aclError aclRet = aclrtSetDevice(deviceId);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtSetDevice %d failed. ERROR: %d", deviceId, aclRet); return nullptr);
    aclRet = aclrtCreateContext(&context, deviceId);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtCreateContext failed. ERROR: %d", aclRet); return nullptr);
    LOG_PRINT("[INFO] GetContextByDevice success.");
    return context;
}

bool SetCurContext(aclrtContext context)
{
    aclError aclRet = aclrtSetCurrentContext(context);
    CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtSetCurrentContext failed. ERROR: %d", aclRet); return false);
    return true;
}

}  // namespace acllite