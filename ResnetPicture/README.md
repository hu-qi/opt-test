# 图片分类（ResNet50）

#### 样例介绍

基于PyTorch框架的ResNet50模型，对*.jpg图片分类，输出各图片Toop5置信度的分类ID、分类名称。

#### 样例下载

可以使用以下两种方式下载，请选择其中一种进行源码准备。

- 命令行方式下载（**下载时间较长，但步骤简单**）。

  ```
  # 登录开发板，HwHiAiUser用户命令行中执行以下命令下载源码仓。    
  cd ${HOME}     
  git clone https://gitee.com/ascend/EdgeAndRobotics.git
  # 切换到样例目录
  cd EdgeAndRobotics/AIApplication/ResnetPicture
  ```

- 压缩包方式下载（**下载时间较短，但步骤稍微复杂**）。

  ```
  # 1. 仓右上角选择 【克隆/下载】 下拉框并选择 【下载ZIP】。     
  # 2. 将ZIP包上传到开发板的普通用户家目录中，【例如：${HOME}/EdgeAndRobotics-master.zip】。      
  # 3. 开发环境中，执行以下命令，解压zip包。      
  cd ${HOME} 
  chmod +x EdgeAndRobotics-master.zip
  unzip EdgeAndRobotics-master.zip
  # 4. 切换到样例目录
  cd EdgeAndRobotics-master/AIApplication/ResnetPicture
  ```

#### 准备环境

1. 以HwHiAiUser用户登录开发板。

2. 设置环境变量。

   ```
   # 配置程序编译依赖的头文件与库文件路径
   export DDK_PATH=/usr/local/Ascend/ascend-toolkit/latest 
   export NPU_HOST_LIB=$DDK_PATH/runtime/lib64/stub
   ```

3. 安装ACLLite库。

   参考[Common安装指导]()安装ACLLite Common库。

   参考[DVPPLite安装指导]()安装DVPPLite库。

   参考[OMExecute安装指导]()安装OMExecute库。

#### 运行样例

1. 以HwHiAiUser用户登录开发板，切换到当前样例目录。
2. 获取PyTorch框架的ResNet50模型（\*.onnx），并转换为昇腾AI处理器能识别的模型（\*.om）。

   ```
   # 为了方便下载，在这里直接给出原始模型下载及模型转换命令,可以直接拷贝执行。
   cd model
   wget https://obs-9be7.obs.cn-east-2.myhuaweicloud.com/003_Atc_Models/resnet50/resnet50.onnx
   wget https://obs-9be7.obs.cn-east-2.myhuaweicloud.com/003_Atc_Models/resnet50/resnet50_DVPP/aipp.cfg
   atc --model=resnet50.onnx --framework=5 --output=resnet50 --input_shape="actual_input_1:1,3,224,224"  --soc_version=Ascend310B4 --insert_op_conf=aipp.cfg
   ```

   atc命令中各参数的解释如下，详细约束说明请参见[《ATC模型转换指南》](https://hiascend.com/document/redirect/CannCommunityAtc)。

   -   --model：ResNet-50网络的模型文件的路径。
   -   --framework：原始框架类型。5表示ONNX。
   -   --output：resnet50.om模型文件的路径。请注意，记录保存该om模型文件的路径，后续开发应用时需要使用。
   -   --input\_shape：模型输入数据的shape。
   -   --soc\_version：昇腾AI处理器的版本。


3. 获取测试图片数据。

   请从以下链接获取该样例的测试图片dog1\_1024\_683.jpg，放在data目录下。

   ```
   cd ../data 
   wget https://obs-9be7.obs.cn-east-2.myhuaweicloud.com/models/aclsample/dog1_1024_683.jpg
   ```

   **注：**若需更换测试图片，则需自行准备测试图片，并将测试图片放到data目录下。

4. 编译样例源码。

   执行以下命令编译样例源码。

   ```
   cd ../scripts 
   bash sample_build.sh
   ```

5. 运行样例。

   执行以下脚本运行样例：

   ```
   bash sample_run.sh
   ```

   执行成功后，在屏幕上的关键提示信息示例如下，提示信息中的top1-5表示图片置信度的前5种类别、index表示类别标识、value表示该分类的最大置信度，class表示所属类别。这些值可能会根据版本、环境有所不同，请以实际情况为准：

   ```
   [INFO] top 1: index[162] value[0.905956] class[beagle]
   [INFO] top 2: index[161] value[0.092549] class[bassetbasset hound]
   [INFO] top 3: index[166] value[0.000758] class[Walker houndWalker foxhound]
   [INFO] top 4: index[167] value[0.000559] class[English foxhound]
   [INFO] top 5: index[163] value[0.000076] class[bloodhound sleuthhound]
   ```

#### 相关操作

- 获取更多样例，请单击[Link](https://gitee.com/ascend/samples/tree/master/inference/modelInference)。
- 获取在线视频课程，请单击[Link](https://www.hiascend.com/edu/courses?activeTab=%E5%BA%94%E7%94%A8%E5%BC%80%E5%8F%91)。
- 获取学习文档，请单击[AscendCL C&C++](https://hiascend.com/document/redirect/CannCommunityCppAclQuick)，查看最新版本的AscendCL推理应用开发指南。
- 查模型的输入输出

  可使用第三方工具Netron打开网络模型，查看模型输入或输出的数据类型、Shape，便于在分析应用开发场景时使用。
