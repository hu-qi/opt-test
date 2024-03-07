# 录音和播音（USB接口）

#### 样例介绍

将USB接口的麦克风连接开发板，再运行本样例实现录音功能。

将USB接口的耳机连接开发板，可通过FFmpeg软件播放录制好的音频。

#### 样例下载

可以使用以下两种方式下载，请选择其中一种进行源码准备。

- 命令行方式下载（**下载时间较长，但步骤简单**）。

  ```
  # 登录开发板，HwHiAiUser用户命令行中执行以下命令下载源码仓。    
  cd ${HOME}     
  git clone https://gitee.com/ascend/EdgeAndRobotics.git
  # 切换到样例目录
  cd EdgeAndRobotics/OnBoardInterface/Audio/USBAudio
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
  cd EdgeAndRobotics-master/OnBoardInterface/Audio/USBAudio
  ```

#### 准备环境

1. 以HwHiAiUser用户登录开发板。

2. 安装FFmpeg。

   ```
   sudo apt-get install ffmpeg libavcodec-dev libswscale-dev libavdevice-dev
   ```

#### 样例运行

1. 以HwHiAiUser用户登录开发板，切换到当前样例目录。

2. 编译样例源码。

   依次执行如下命令：

   ```
   # 执行编译命令
   gcc main.c -o main -lavutil -lavdevice -lavformat -lavcodec
   ```

   编译命令执行成功后，在USBAudio样例目录下生成可执行文件main。

3. 运行样例，进行录音。

   执行如下命令查看录音设备编号：

   ```
   arecord -l
   ```

   终端回显信息示例如下，其中**card 0**中的**0**表示录音设备编号：

   ```
   **** List of CAPTURE Hardware Devices ****
   card 0: X5 [EPOS IMPACT X5], device 0: USB Audio [USB Audio]
     Subdevices: 0/1
     Subdevice #0: subdevice #0
   ```

   运行可执行文件，其中0表示录音设备编号，需根据实际编码填写：

   ```
   ./main plughw:0
   ```

   运行可执行文件后，您可以使用麦克风录音，录音结束时，在终端界面输入**over**退出。

   录音成功后，在USBAudio样例目录下生成音频文件audio.pcm。

4. 播音。

   执行如下命令，使用FFmpeg软件播音：

   ```
   ffplay -ar 44100 -ac 2 -f s16le audio.pcm
   ```

#### 相关操作

暂无