# Camera图像获取（USB接口）

#### 样例介绍

通过USB接口连接树莓派Camera与开发板，从树莓派Camera获取图像、并处理为YUV图像。

#### 样例下载

可以使用以下两种方式下载，请选择其中一种进行源码准备。

- 命令行方式下载（**下载时间较长，但步骤简单**）。

  ```
  # 登录开发板，HwHiAiUser用户命令行中执行以下命令下载源码仓。    
  cd ${HOME}     
  git clone https://gitee.com/ascend/EdgeAndRobotics.git
  # 切换到样例目录
  cd EdgeAndRobotics/OnBoardInterface/Camera/USBCamera
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
  cd EdgeAndRobotics-master/OnBoardInterface/Camera/USBCamera
  ```

#### 准备环境

1. 以HwHiAiUser用户登录开发板。

2. 安装FFmpeg。

   ```
   sudo apt-get install ffmpeg libavcodec-dev libswscale-dev libavdevice-dev
   ```

3. 安装OpenCV。

   ```
   sudo apt-get install libopencv-dev
   ```

#### 样例运行

1. 以HwHiAiUser用户登录开发板，切换到当前样例目录。

2. 编译样例源码。

   ```
   # 执行编译命令
   g++ main.cpp -o main -lavutil -lavformat -lavcodec -lavdevice
   ```

   编译命令执行成功后，在USBCamera样例目录下生成可执行文件main。

3. 运行样例，从Camera获取图像。

   运行可执行文件，其中/dev/video0表示Camera设备，需根据实际情况填写：

   ```
   ./main /dev/video0
   ```

   运行成功后，在USBCamera样例目录下生成yuyv422格式、1280*720分辨率的out.yuv文件。

   **注意**：当把一个摄像头插入开发板后，执行ls /dev/vi*命令可看到摄像头的vedio节点。这里出现了两个设备节点：/dev/video0、/dev/video1，是因为一个是图像/视频采集，一个是metadata采集，因此本样例中在运行可执行文件时，选择图像/视频采集的设备节点/dev/video0。

4. 检查从Camera获取的图像。

   执行如下命令，使用FFmpeg软件查看图像：

   ```
   ffplay -pix_fmt yuyv422 -video_size 1280*720 out.yuv
   ```

#### 相关操作

暂无