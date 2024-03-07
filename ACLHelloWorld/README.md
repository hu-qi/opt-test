# Hello World!

#### 样例介绍

基于AscendCL接口的HelloWorld应用，仅包含运行管理资源申请与释放功能。

本样例涉及以下运行管理资源：

- Device：指安装了昇腾AI处理器的硬件设备，提供NN（Neural Network）计算能力。
- Context：Context在Device下，一个Context一定属于一个唯一的Device。Context作为一个容器，管理了所有对象（包括Stream、设备内存等）的生命周期。
- Stream：Stream用于维护一些异步操作的执行顺序，确保按照应用程序中的代码调用顺序在Device上执行。Stream是Device上的执行流，在同一个Stream中的任务执行严格保序。

#### 样例下载

可以使用以下两种方式下载，请选择其中一种进行源码准备。

   - 命令行方式下载（**下载时间较长，但步骤简单**）。

     ```
     # 登录开发板，HwHiAiUser用户命令行中执行以下命令下载源码仓。    
     cd ${HOME}     
     git clone https://gitee.com/ascend/EdgeAndRobotics.git
     # 切换到样例目录
     cd EdgeAndRobotics/AIApplication/ACLHelloWorld
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
     cd EdgeAndRobotics-master/AIApplication/ACLHelloWorld
     ```


**样例的代码目录说明如下：**

```
|--- scripts       
    |--- sample_build.sh   // 将样例源码编译成可执行文件的脚本
    |--- sample_run.sh     // 运行可执行文件的脚本
|--- src
    |--- CMakeLists.txt    // cmake编辑脚本
    |--- main.cpp          // 样例源码
```

#### 准备环境

1. 以HwHiAiUser用户登录开发板。

2. 设置环境变量。

   ```
   # 配置程序编译依赖的头文件与库文件路径
   export DDK_PATH=/usr/local/Ascend/ascend-toolkit/latest 
   export NPU_HOST_LIB=$DDK_PATH/runtime/lib64/stub
   ```


#### 样例运行

1. 以HwHiAiUser用户登录开发板，切换到当前样例目录。

2. 编译样例源码。

   执行以下命令编译样例源码。

   ```
   cd scripts 
   bash sample_build.sh
   ```

3. 运行样例。

   执行以下脚本运行样例：

   ```
   bash sample_run.sh
   ```

   执行成功后，在终端屏幕上的提示信息示例如下：

   ```
   [INFO] The sample starts to run
   [INFO] Init Ascend NPU Success, NPU-ID:0
   [INFO] Hello! Welcome to Ascend World!
   [INFO] Reset Ascend NPU Success.
   ```

#### 相关操作

-   获取在线视频课程，请单击[Link](https://www.hiascend.com/edu/courses?activeTab=%E5%BA%94%E7%94%A8%E5%BC%80%E5%8F%91)。
-   获取学习文档，请单击[AscendCL C&C++](https://hiascend.com/document/redirect/CannCommunityCppAclQuick)或[AscendCL Python](https://hiascend.com/document/redirect/CannCommunityPyaclQuick)，查看最新版本的AscendCL推理应用开发指南。