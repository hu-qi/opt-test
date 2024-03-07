### DVPPLite模块API说明
该模块为资源管理及公共函数模块，主要包含以下功能：
1. 视频解码
2. 视频编码
3. 图像编解码及VPC处理

#### 视频解码
- VideoRead类：命名空间为acllite
    1. **VideoRead(const std::string& videoName, int32_t deviceId = 0, aclrtContext context = nullptr)**
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

    2. **~VideoRead()**
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

    5. **bool Set(StreamProperty propId, int value)**
        - 功能说明    
            设置解码属性
        - 参数说明     
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | propId| 输入 | 需要设置的属性，StreamProperty结构数据详见[StreamProperty](./common.md#公共数据结构) |
            | value| 输入 | 需要设置的属性的具体值|
        - 返回值说明    
            返回true表示成功，返回false表示失败。

    6. **uint32_t Get(StreamProperty propId)**
        - 功能说明    
            获取解码视频流信息
        - 参数说明     
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | propId| 输入 | 需要获取的属性，StreamProperty结构数据详见[StreamProperty](./common.md#公共数据结构) |
        - 返回值说明    
            返回获取到的属性的具体值。

    7. **void Release()**
        - 功能说明    
            资源释放函数
        - 参数说明    
            无
        - 返回值说明    
            无

#### 视频编码

- VideoWrite类：命名空间为acllite
    1. **VideoWrite(std::string outFile, uint32_t maxWidth, uint32_t maxHeight, int32_t deviceId = 0)**
        - 功能说明    
            构造函数，创建一个VideoWrite对象
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | outFile | 输入 | h264/h264文件地址 |
            | maxWidth | 输入 | 输出文件宽 |
            | maxHeight | 输入 | 输出文件高 |
            | deviceId | 输入 | 设备id，缺省为0 |
        - 返回值说明   
            无

    2. **~VideoWrite()**
        - 功能说明    
            析构函数，销毁VideoRead对象
        - 参数说明    
            无
        - 返回值说明    
            无

    3. **bool Write(ImageData& image)**
        - 功能说明    
            写入一帧图像数据进行编码
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | image| 输出 | 写入的图像数据和属性，ImageData结构数据详见[ImageData](./common.md#公共数据结构) |
        - 返回值说明    
            返回true表示成功，返回false表示失败。

    4. **bool IsOpened()**
        - 功能说明    
            判断写入文件是否已经打开
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

#### 图像编解码及VPC处理
- ImageProc类：命名空间为acllite
    1. **ImageProc(int deviceId = 0)**
        - 功能说明    
            构造函数，创建一个ImageProc对象
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | deviceId | 输入 | 设备id，缺省为0 |
        - 返回值说明    
            无

    2. **~ImageProc()**
        - 功能说明    
            析构函数，销毁ImageProc对象
        - 参数说明    
            无
        - 返回值说明    
            无

    3. **ImageData Read(const std::string& fileName, acldvppPixelFormat imgFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420)**
        - 功能说明    
            读取一帧图片，将JPG图片解码为YUV
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | fileName | 输入 | 输入的JPG文件路径 |
            | imgFormat | 输入 | 解码后的图像格式，默认为YUV420SP |

        - 返回值说明    
            返回解码后的图像数据和属性，ImageData结构数据详见[ImageData](./common.md#公共数据结构)。

    4. **void Resize(ImageData& src, ImageData& dst, ImageSize dsize, ResizeType type = RESIZE_COMMON)**
        - 功能说明    
            对图片进行缩放
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | src | 输入 | 输入的图像数据和属性，ImageData结构数据详见[ImageData](./common.md#公共数据结构) |
            | dst | 输出 | 输出的图像数据和属性，ImageData结构数据详见[ImageData](./common.md#公共数据结构) |
            | dsize | 输入 | 输出图片分辨率，ImageSize结构数据详见[ImageSize](./common.md#公共数据结构) |
            | type | 输入 | 缩放类型，ResizeType结构数据详见[ResizeType](./common.md#公共数据结构) |
        - 返回值说明    
            无

    5. **bool Write(const std::string& fileName, ImageData& img)**
        - 功能说明    
            对图片进行编码，写成图片文件
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | fileName | 输入 | 输出的JPG文件路径 |
            | img | 输入 | 输入图像数据和属性，ImageData结构数据详见[ImageData](./common.md#公共数据结构) |
        - 返回值说明    
            返回true表示成功，返回false表示失败。