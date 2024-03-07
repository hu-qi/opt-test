### OMExcute模块API说明
该模块为模型管理模块，主要包含模型加载、输入输出创建、推理等功能。

- ModelProc类：命名空间为acllite
    1. **ModelProc()**
        - 功能说明    
            构造函数，创建一个ModelProc对象
        - 参数说明
            无
        - 返回值说明   
            无

    2. **~ModelProc()**
        - 功能说明 
            析构函数，销毁ModelProc对象
        - 参数说明
            无
        - 返回值说明
            无

    3. **void DestroyResource()**
        - 功能说明 
            销毁模型处理过程中的相关资源
        - 参数说明
            无
        - 返回值说明
            无

    4. **bool Load(const std::string& modelPath)**
        - 功能说明 
            模型加载函数
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | modelPath | 输入 | 模型路径 |
        - 返回值说明
            返回true表示成功，返回false表示失败。

    5. **bool CreateInput(void \*input, uint32_t size)**
        - 功能说明 
            创建模型输入（模型单输入场景）
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | input | 输入 | 输入数据 |
            | size | 输入 | 输入数据大小 |
        - 返回值说明
            返回true表示成功，返回false表示失败。

    6. **bool CreateInput(void \*input1, uint32_t input1size, void \*input2, uint32_t input2size)**
        - 功能说明 
            创建模型输入（模型两个输入场景）
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | input1 | 输入 | 首个输入数据 |
            | input1size | 输入 | 首个输入数据大小 |
            | input2 | 输入 | 第二个输入数据大小 |
            | input2size | 输入 | 第二个输入数据大小 |
        - 返回值说明
            返回true表示成功，返回false表示失败。

    7. **bool CreateInput(std::vector<DataInfo>& inputData)**
        - 功能说明 
            创建模型输入（模型多输入场景）
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | inputData | 输入 | 输入数据，vector结构 |
        - 返回值说明
            返回true表示成功，返回false表示失败。

    8. **bool Execute(std::vector<InferenceOutput>& inferOutputs)**
        - 功能说明 
            模型执行函数
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | inferOutputs | 输出 | 输出数据，vector结构 |
        - 返回值说明
            返回true表示成功，返回false表示失败。

    9. **void DestroyInput()**
        - 功能说明 
            释放模型输入
        - 参数说明
            无
        - 返回值说明
            无

    10. **void DestroyOutput()**
        - 功能说明 
            释放模型输出
        - 参数说明
            无
        - 返回值说明
            无

    11. **void Unload()**
        - 功能说明 
            卸载模型
        - 参数说明
            无
        - 返回值说明
            无