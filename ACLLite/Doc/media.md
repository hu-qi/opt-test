### Media模块API说明
该模块为媒体处理模块，当前主要包含以下功能：
1. USB摄像头读取

#### USB摄像头读取
- CameraRead类：命名空间为acllite
    1. **CameraRead(const std::string& videoName, int32_t deviceId = 0, aclrtContext context = nullptr)**
        - 功能说明    
            构造函数，创建一个VideoRead对象
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | videoName | 输入 | MP4视频/RTSP流地址 |
            | deviceId | 输入 | 设备id，缺省为0 |
            | context | 输入 | 解码器进行vdec时使用的context；缺省为nullptr，此时使用当前线程的context |
        - 返回值说明   
            无

    2. **~CameraRead()**
        - 功能说明 
            析构函数，销毁VideoRead对象
        - 参数说明
            无
        - 返回值说明
            无

    3. **bool Read(ImageData& image)**
        - 功能说明 
            获取要被处理的一帧图像数据
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | image| 输出 | 读取的图像数据和属性，ImageData结构数据详见[ImageData](./common.md#公共数据结构) |
        - 返回值说明
            返回true表示成功，返回false表示失败。

    4. **bool IsOpened()**
        - 功能说明 
            判断摄像头或者视频流是否已经打开
        - 参数说明
            无
        - 返回值说明
            返回true表示成功，返回false表示失败。

    5. **void Release()**
        - 功能说明 
            资源释放函数
        - 参数说明
            无
        - 返回值说明
            无

    6. **uint32_t Get(StreamProperty propId)**
        - 功能说明    
            获取解码视频流信息
        - 参数说明     
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | propId| 输入 | 需要获取的属性，StreamProperty结构数据详见[StreamProperty](./common.md#公共数据结构) |
        - 返回值说明    
            返回获取到的属性的具体值。