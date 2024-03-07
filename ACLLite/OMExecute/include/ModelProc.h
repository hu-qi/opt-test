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
#ifndef MODEL_PROC_H
#define MODEL_PROC_H
#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include "acl/acl.h"
#include "acllite_common/Common.h"

namespace acllite {

class ModelProc {
public:
    ModelProc();
    ~ModelProc();
    void DestroyResource();
    bool Load(const std::string& modelPath);
    bool CreateInput(void* input, uint32_t size);
    bool CreateInput(void* input1, uint32_t input1size,
        void* input2, uint32_t input2size);
    bool CreateInput(std::vector<DataInfo>& inputData);
    bool Execute(std::vector<InferenceOutput>& inferOutputs);
    void DestroyInput();
    void DestroyOutput();
    void Unload();
private:
    bool AddDatasetBuffer(aclmdlDataset* dataset,
        void* buffer, uint32_t bufferSize);
    bool GetOutputItem(InferenceOutput& out, uint32_t idx);
private:
    aclrtRunMode runMode_;
    bool loadFlag_;
    bool isReleased_;
    uint32_t modelId_;
    std::string modelPath_;
    aclmdlDesc *modelDesc_;
    aclmdlDataset *input_;
    aclmdlDataset *output_;
    size_t outputsNum_;
    std::vector<void*> outputBuf_;
    std::vector<size_t> outputSize_;
    std::vector<aclDataBuffer*> outputDataBuf_;
};
}  // namespace acllite
#endif