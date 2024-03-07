/**
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2024. All rights reserved.
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

* File ModelProc.cpp
* Description: handle model process
*/
#include <iostream>
#include "ModelProc.h"
using namespace std;
namespace acllite {
ModelProc::ModelProc():loadFlag_(false), isReleased_(false),
    modelId_(0), modelPath_(""), modelDesc_(nullptr),
    input_(nullptr), output_(nullptr), outputsNum_(0)
{
    aclrtGetRunMode(&runMode_);
}

ModelProc::~ModelProc()
{
    DestroyResource();
}

void ModelProc::DestroyResource()
{
    if (isReleased_) {
        return;
    }
    Unload();
    isReleased_ = true;
}

bool ModelProc::Load(const string& modelPath)
{
    modelPath_.assign(modelPath.c_str());
    if (loadFlag_) {
        LOG_PRINT("[ERROR] %s is loaded already", modelPath_.c_str());
        return false;
    }
    aclError aclRet = aclmdlLoadFromFile(modelPath_.c_str(), &modelId_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Load model(%s) from file return %d",
                          modelPath_.c_str(), aclRet);
        return false;
    }
    loadFlag_ = true;

    modelDesc_ = aclmdlCreateDesc();
    if (modelDesc_ == nullptr) {
        LOG_PRINT("[ERROR] Create model(%s) description failed", modelPath_.c_str());
        return false;
    }
    aclRet = aclmdlGetDesc(modelDesc_, modelId_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Get model(%s) description failed",
                          modelPath_.c_str());
        return false;
    }

    if (modelDesc_ == nullptr) {
        LOG_PRINT("[ERROR] Create output failed for no model(%s) description",
                          modelPath_.c_str());
        return false;
    }

    output_ = aclmdlCreateDataset();
    if (output_ == nullptr) {
        LOG_PRINT("[ERROR] Create output failed for create dataset error");
        return false;
    }

    outputsNum_ = aclmdlGetNumOutputs(modelDesc_);
    for (size_t i = 0; i < outputsNum_; ++i) {
        size_t bufSize = aclmdlGetOutputSizeByIndex(modelDesc_, i);
        void *outputBuffer = nullptr;
        aclRet = aclrtMalloc(&outputBuffer, bufSize,
                                   ACL_MEM_MALLOC_NORMAL_ONLY);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] Create output failed for malloc "
                      "device failed, size %d", (int)bufSize);
            return false;
        }
        aclDataBuffer* dataBuf = aclCreateDataBuffer(outputBuffer, bufSize);
        if (dataBuf == nullptr) {
            LOG_PRINT("[ERROR] Create data buffer error");
            return false;
        }
        aclRet = aclmdlAddDatasetBuffer(output_, dataBuf);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] Add dataset buffer error %d", aclRet);
            aclRet = aclDestroyDataBuffer(dataBuf);
            if (aclRet != ACL_SUCCESS) {
                LOG_PRINT("[ERROR] Destroy dataset buffer error %d", aclRet);
            }
            dataBuf = nullptr;
            return false;
        }
        outputBuf_.push_back(outputBuffer);
        outputSize_.push_back(bufSize);
        outputDataBuf_.push_back(dataBuf);
    }
    LOG_PRINT("[INFO] Load model %s success", modelPath_.c_str());

    return true;
}

bool ModelProc::CreateInput(void *input, uint32_t size)
{
    vector<DataInfo> inputData = {{input, size}};
    return CreateInput(inputData);
}

bool ModelProc::CreateInput(void *input1, uint32_t input1size,
                            void* input2, uint32_t input2size)
{
    vector<DataInfo> inputData = {{input1, input1size}, {input2, input2size}};
    return CreateInput(inputData);
}

bool ModelProc::CreateInput(vector<DataInfo>& inputData)
{
    uint32_t dataNum = aclmdlGetNumInputs(modelDesc_);
    if (dataNum == 0) {
        LOG_PRINT("[ERROR] Create input failed for no input data");
        return false;
    }

    if (dataNum != inputData.size()) {
        LOG_PRINT("[ERROR] Create input failed for wrong input nums");
        return false;
    }
    input_ = aclmdlCreateDataset();
    if (input_ == nullptr) {
        LOG_PRINT("[ERROR] Create input failed for create dataset failed");
        return false;
    }

    for (uint32_t i = 0; i < inputData.size(); i++) {
        size_t modelInputSize = aclmdlGetInputSizeByIndex(modelDesc_, i);
        if (modelInputSize != inputData[i].size) {
            LOG_PRINT("[WARNING] Input size verify failed "
                "input[%d] size: %ld, provide size : %d",
                i, modelInputSize, inputData[i].size);
        }
        bool ret = AddDatasetBuffer(input_,
                                    inputData[i].data,
                                    inputData[i].size);
        if (!ret) {
            LOG_PRINT("[ERROR] Create input failed for "
                "add dataset buffer error %d", ret);
            return false;
        }
    }

    return true;
}

bool ModelProc::AddDatasetBuffer(aclmdlDataset *dataset,
                                 void* buffer, uint32_t bufferSize)
{
    aclDataBuffer* dataBuf = aclCreateDataBuffer(buffer, bufferSize);
    if (dataBuf == nullptr) {
        LOG_PRINT("[ERROR] Create data buffer error");
        return false;
    }

    aclError ret = aclmdlAddDatasetBuffer(dataset, dataBuf);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Add dataset buffer error %d", ret);
        ret = aclDestroyDataBuffer(dataBuf);
        if (ret != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] Destroy dataset buffer error %d", ret);
        }
        dataBuf = nullptr;
        return false;
    }

    return true;
}

bool ModelProc::Execute(vector<InferenceOutput>& inferOutputs)
{
    aclError aclRet = aclmdlExecute(modelId_, input_, output_);
    if (aclRet != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Execute model(%s) error:%d", modelPath_.c_str(), aclRet);
        return false;
    }

    for (uint32_t i = 0; i < outputsNum_; i++) {
        InferenceOutput out;
        bool ret = GetOutputItem(out, i);
        if (!ret) {
            LOG_PRINT("[ERROR] Get the %dth interference output failed, "
                "error: %d", i, ret);
            return ret;
        }
        inferOutputs.push_back(out);
    }
    DestroyInput();
    return true;
}

bool ModelProc::GetOutputItem(InferenceOutput& out, uint32_t idx)
{
    aclError aclRet;
    if (runMode_ == ACL_DEVICE) {
        out.data = SHARED_PTR_DEV_BUF(outputBuf_[idx]);
        out.size = outputSize_[idx];
        void *outputBuffer = nullptr;
        aclRet = aclrtMalloc(&outputBuffer, outputSize_[idx], ACL_MEM_MALLOC_NORMAL_ONLY);
        CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtMalloc failed. ERROR: %d", aclRet); return false);
        outputBuf_[idx] = outputBuffer;
        aclRet = aclUpdateDataBuffer(outputDataBuf_[idx], outputBuffer, outputSize_[idx]);
        CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclUpdateDataBuffer failed. ERROR: %d", aclRet); return false);
    } else {
        void* hostData = nullptr;
        aclRet = aclrtMallocHost(&hostData, outputSize_[idx]);
        CHECK_RET(aclRet == ACL_SUCCESS, LOG_PRINT("[ERROR] aclrtMallocHost failed. ERROR: %d", aclRet); return false);
        aclRet = aclrtMemcpy(hostData, outputSize_[idx], outputBuf_[idx], outputSize_[idx], ACL_MEMCPY_DEVICE_TO_HOST);
        if (aclRet != ACL_SUCCESS) {
            LOG_PRINT("[ERROR] aclrtMemcpy failed. ERROR: %d", aclRet);
            aclrtFreeHost(hostData);
            return false;
        }
        out.data = SHARED_PTR_HOST_BUF(hostData);
        out.size = outputSize_[idx];
    }
    return true;
}

void ModelProc::DestroyInput()
{
    if (input_ == nullptr) {
        return;
    }

    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(input_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(input_, i);
        aclDestroyDataBuffer(dataBuffer);
        dataBuffer = nullptr;
    }
    aclmdlDestroyDataset(input_);
    input_ = nullptr;
}

void ModelProc::DestroyOutput()
{
    if (output_ == nullptr) {
        return;
    }

    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(output_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(output_, i);
        void* data = aclGetDataBufferAddr(dataBuffer);
        (void)aclrtFree(data);
        (void)aclDestroyDataBuffer(dataBuffer);
        dataBuffer = nullptr;
    }

    (void)aclmdlDestroyDataset(output_);
    output_ = nullptr;
}

void ModelProc::Unload()
{
    if (!loadFlag_) {
        LOG_PRINT("[INFO] Model(%s) had not been loaded or unload already",
                         modelPath_.c_str());
        return;
    }
    DestroyOutput();
    aclError ret = aclmdlUnload(modelId_);
    if (ret != ACL_SUCCESS) {
        LOG_PRINT("[ERROR] Unload model(%s) error:%d", modelPath_.c_str(), ret);
    }

    if (modelDesc_ != nullptr) {
        (void)aclmdlDestroyDesc(modelDesc_);
        modelDesc_ = nullptr;
    }
    loadFlag_ = false;
    LOG_PRINT("[INFO] Unload model %s success", modelPath_.c_str());
}
}  // namespace acllite
