## 目录

  - [工具介绍](#工具介绍)
  - [获取源码包](#获取源码包) 
  - [编译安装](#编译安装)
  - [更新说明](#更新说明)
  - [已知issue](#已知issue)
    
## 工具介绍

Common对一些常用操作以及结构体进行了封装操作，通过安装Common动态库，可以更方便的编写代码。

## 获取源码包
    
 可以使用以下两种方式下载，请选择其中一种进行源码准备。

 - 命令行方式下载（下载时间较长，但步骤简单）。

   ```    
   # 开发环境，非root用户命令行中执行以下命令下载源码仓。    
   cd ${HOME}     
   git clone https://gitee.com/ascend/ACLLite.git
   ```  
 - 压缩包方式下载（下载时间较短，但步骤稍微复杂）。   
   **注：如果需要下载其它版本代码，请先请根据前置条件说明进行samples仓分支切换。**   
   ``` 
   # 1. ACLLite仓右上角选择 【克隆/下载】 下拉框并选择 【下载ZIP】。    
   # 2. 将ZIP包上传到开发环境中的普通用户家目录中，【例如：${HOME}/acllite-master.zip】。     
   # 3. 开发环境中，执行以下命令，解压zip包。     
   cd ${HOME}    
   unzip acllite-master.zip
   ```

## 第三方依赖安装

 设置环境变量，配置程序编译依赖的头文件，库文件路径。“$HOME/Ascend”请替换“Ascend-cann-toolkit”包的实际安装路径。

   ```
    export DDK_PATH=/usr/local/Ascend/ascend-toolkit/latest
    export NPU_HOST_LIB=$DDK_PATH/runtime/lib64/stub
   ```
    
## 编译安装

  - 库编译

    执行以下命令，执行编译命令，开始库编译。
    ```
    cd $HOME/ACLite/Common
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr .. -DCMAKE_C_COMPILER=gcc -DCMAKE_SKIP_RPATH=TRUE
    make
    ```
  - 库安装

    执行安装命令，开始安装动态库。
    ```
    sudo make install
    ```
    
    执行成功后，生成的so文件会被拷贝安装到/usr/lib目录下，对应的头文件会被拷贝安装到/usr/include/acllite_common目录下。

## 更新说明
  | 时间 | 更新事项 |
  |----|------|
  | 2024/01/04 | 新增README.md |
  

## 已知issue

  暂无
