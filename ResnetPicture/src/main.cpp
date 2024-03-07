#include <cmath>
#include <dirent.h>
#include <string.h>
#include <map>
#include "acllite_dvpp_lite/ImageProc.h"
#include "acllite_om_execute/ModelProc.h"
#include "label.h"
using namespace std;
using namespace acllite;

int main()
{
    AclLiteResource aclResource;
    bool ret = aclResource.Init();
    CHECK_RET(ret, LOG_PRINT("[ERROR] InitACLResource failed."); return 1);

    ImageProc imageProcess;
    ModelProc modelProc;
    ret = modelProc.Load("../model/resnet50.om");
    CHECK_RET(ret, LOG_PRINT("[ERROR] load model resnet50.om failed."); return 1);
    ImageData src = imageProcess.Read("../data/dog1_1024_683.jpg");
    CHECK_RET(src.size, LOG_PRINT("[ERROR] Read image failed."); return 1);

    ImageData dst;
    ImageSize dsize(224,224);

    imageProcess.Resize(src, dst, dsize);
    ret = modelProc.CreateInput(static_cast<void *>(dst.data.get()), dst.size);
    CHECK_RET(ret, LOG_PRINT("[ERROR] Create model input failed."); return 1);
    vector<InferenceOutput> inferOutputs;
    modelProc.Execute(inferOutputs);
    CHECK_RET(ret, LOG_PRINT("[ERROR] model execute failed."); return 1);

    uint32_t dataSize = inferOutputs[0].size;
    // get result from output data set
    float* outData = static_cast<float*>(inferOutputs[0].data.get());
    if (outData == nullptr) {
        LOG_PRINT("get result from output data set failed.");
        return 1;
    }
    map<float, unsigned int, greater<float> > resultMap;
    for (uint32_t j = 0; j < dataSize / sizeof(float); ++j) {
        resultMap[*outData] = j;
        outData++;
    }

    uint32_t topConfidenceLevels = 5;
    double totalValue = 0.0;
    for (auto it = resultMap.begin(); it != resultMap.end(); ++it) {
        totalValue += exp(it->first);
    }

    int cnt = 0;
    for (auto it = resultMap.begin(); it != resultMap.end(); ++it) {
        // print top 5
        if (++cnt > topConfidenceLevels) {
            break;
        }
        LOG_PRINT("[INFO] top %d: index[%d] value[%lf] class[%s]", cnt, it->second,
                         exp(it->first) / totalValue, label[it->second].c_str());
    }
    outData = nullptr;
    return 0;
}