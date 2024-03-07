### common模块API说明
该模块为资源管理及公共函数模块，主要包含以下功能：
1. 资源管理
2. 公共函数
3. 工具宏
4. 公共数据结构

#### 资源管理
- AclLiteResource类：命名空间为acllite，所属类为AclLiteResource
    1. **AclLiteResource(int32_t devId = 0)**
        - 功能说明    
            构造函数，创建一个AclLiteResource对象
        - 参数说明
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | devId | 输入 | 设备id |
        - 返回值说明   
            无

    2. **~AclLiteResource()**
        - 功能说明    
            析构函数，销毁AclLiteResource对象
        - 参数说明
            无
        - 返回值说明   
            无

    3. **bool Init()**
        - 功能说明    
            初始化函数，初始化Acl相关资源device、context
        - 参数说明
            无
        - 返回值说明   
            返回true表示成功，返回false表示失败。

    4. **aclrtContext GetContext()**
        - 功能说明    
            获取程序运行的context
        - 参数说明   
            无
        - 返回值说明   
            返回获取到的context；返回nullptr表示无效context，返回非nullptr表示有效context

    5. **void Release()**
        - 功能说明    
            资源释放函数，释放Acl相关资源device、context
        - 参数说明   
            无
        - 返回值说明    
            无

#### 公共函数
- 其它模块需要使用的公共函数：命名空间为acllite，未封装为类
    1. **void SaveBinFile(const std::string& filename, const void\* data, uint32_t size)**
        - 功能说明    
            将数据存为二进制文件
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | filename | 输入 | 文件名 |
            | data | 输入 | 待保存数据 |
            | size | 输入 | 待保存数据大小 |
        - 返回值说明   
            无

    2. **bool ReadBinFile(const std::string& fileName, void\*& data, uint32_t& size)**
        - 功能说明    
            读取二进制文件
        - 参数说明      
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | filename | 输入 | 文件名 |
            | data | 输出 | 待读取数据 |
            | size | 输入 | 待读取数据大小 |
        - 返回值说明   
            返回true表示成功，返回false表示失败。

    3. **void\* CopyDataToDevice(void\* data, uint32_t size);**
        - 功能说明    
            将数据拷贝到Device侧
        - 参数说明      
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | data| 输入 | 待拷贝数据 |
            | size| 输入 | 待拷贝数据大小 |
        - 返回值说明   
            返回拷贝后的Device内存地址；返回nullptr表示拷贝失败，返回非nullptr表示拷贝成功

    4. **aclrtContext GetContextByDevice(int32_t devId)**
        - 功能说明    
            获取对应device上的context
        - 参数说明    
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | devId | 输入 | 设备id |
        - 返回值说明   
            返回获取到的context；返回nullptr表示无效context，返回非nullptr表示有效context

    5. **bool SetCurContext(aclrtContext context);**
        - 功能说明    
            设置当前线程的context
        - 参数说明     
            | 参数名 | 输入/输出  |  说明 |
            |---|---|---|
            | context | 输入 | 待设置的context |
        - 返回值说明   
            返回true表示成功，返回false表示失败。

#### 工具宏
- 宏封装函数：无命名空间
    1. **ALIGN_UP(num, align)**
        - 功能说明    
            计算对齐后的数值
        - 参数说明           
            | 参数名 |  说明 |
            |---|---|
            | num | 原始数值 |
            | align| 要对齐的数 |

    2. **ALIGN_UP2(num, align)**
        - 功能说明    
            将数据按2对齐
        - 参数说明       
            | 参数名 | 说明 |
            |---|---|
            | num | 原始数值 |
            | align| 要对齐的数 |

    3. **ALIGN_UP16(num, align)**
        - 功能说明    
            将数据按16对齐
        - 参数说明    
            | 参数名 | 说明 |
            |---|---|
            | num | 原始数值 |
            | align| 要对齐的数 |

    4. **ALIGN_UP64(num, align)**
        - 功能说明    
            将数据按64对齐
        - 参数说明        
            | 参数名 | 说明 |
            |---|---|
            | num | 原始数值 |
            | align| 要对齐的数 |

    5. **ALIGN_UP128(num, align)**
        - 功能说明    
            将数据按128对齐
        - 参数说明        
            | 参数名 | 说明 |
            |---|---|
            | num | 原始数值 |
            | align| 要对齐的数 |

    6. **YUV420SP_SIZE(width, height)**
        - 功能说明    
            计算YUV420SP图片数据大小
        - 参数说明               
            | 参数名 | 说明 |
            |---|---|
            | width | 图片宽 |
            | height | 图片高 |

    7. **YUV422P_SIZE(width, height)**
        - 功能说明    
            计算YUV422P图片数据大小
        - 参数说明              
            | 参数名 | 说明 |
            |---|---|
            | width | 图片宽 |
            | height | 图片高 |

    8. **SHARED_PTR_U8_BUF(buf)**
        - 功能说明    
            新建指向一般内存的智能指针
        - 参数说明          
            | 参数名 |  说明 |
            |---|---|
            | buf | 指向一般内存的指针，用delete释放  |

    9. **SHARED_PTR_HOST_BUF(buf)**
        - 功能说明    
            新建指向HOST内存的智能指针
        - 参数说明           
            | 参数名 |  说明 |
            |---|---|
            | buf | 指向HOST内存的指针，用aclrtFreeHost释放  |

    10. **SHARED_PTR_DEV_BUF(buf)**
        - 功能说明    
            新建指向DEV内存的智能指针
        - 参数说明  
            | 参数名 |  说明 |           
            |---|---|
            | buf | 指向DEV内存的指针，用aclrtFree释放  |

    11. **SHARED_PTR_DVPP_BUF(buf)**
        - 功能说明    
            新建指向DVPP内存的智能指针
        - 参数说明       
            | 参数名 | 说明 |
            |---|---|
            | buf | 指向DVPP内存的指针，用acldvppFree释放  |

    12. **CHECK_RET(cond, return_expr)**
        - 功能说明    
            函数返回值判断
        - 参数说明
            | 参数名 |  说明 |
            |---|---|
            | cond | 执行语句，判断返回失败则执行return_expr  |
            | return_expr | 异常返回执行操作  |

    13. **LOG_PRINT(message, ...)**
        - 功能说明    
            打印日志
        - 参数说明
            | 参数名 | 说明 |
            |---|---|
            | message | 打印的日志内容  |
    
#### 公共数据结构
- ACLLite中使用到的全局变量：命名空间为acllite
    1. **Result**
        - 结构展示
            ```
            typedef int Result;
            ```
        - 说明
            用来表述返回值，一般用于函数定义或调用

    2. **RES_OK**
        - 结构展示
            ```
            const int RES_OK = 0;
            ```
        - 说明
            用来表述正常返回

    3. **RES_ERROR**
        - 结构展示
            ```
            const int RES_ERROR = 1;
            ```
        - 说明
            用来表述异常返回

- ACLLite中需要使用到的数据结构：命名空间为acllite
    1. **struct DataInfo**
        - 结构展示
            ```
            struct DataInfo {
                void* data;
                uint32_t size;
            };
            ```
        - 功能说明    
            数据信息，存储数据内容及数据大小，用来生成模型输入输出
        - 数据成员说明
            | 成员名 |  说明 |
            |---|---|
            | data | 数据 |
            | size | 数据大小 |

    2. **struct InferenceOutput**
        - 结构展示
            ```
            struct InferenceOutput {
                std::shared_ptr<void> data = nullptr;
                uint32_t size;
            };
            ```
        - 功能说明    
            存放模型推理结果数据
        - 数据成员说明
            | 成员名 |  说明 |
            |---|---|
            | data | 数据，格式为共享指针，默认指向nullptr |
            | size | 数据大小 |

    3. **struct ImageSize**
        - 结构展示
            ```
            struct ImageSize {
                uint32_t width = 0;
                uint32_t height = 0;
                ImageSize() {}
                ImageSize(uint32_t x, uint32_t y) : width(x), height(y) {}
            };
            ```
        - 功能说明    
            图像宽高组合，主要用于VPC中目标图片宽高取值
        - 数据成员说明
            | 成员名 |  说明 |
            |---|---|
            | width | 图像宽，默认为0 |
            | height | 图像高，默认为0 |
        - 构造方式
            ```
            uint32_t x = 1920;
            uint32_t y = 1080;
            // 1. 通过构造函数
            ImageSize z1 = ImageSize(x, y);

            // 2. 通过结构体构造
            ImageSize z2；
            z2.width = x;
            z2.height = y;
            ```

    4. **struct ImageData**
        - 结构展示
            ```
            struct ImageData {
                std::shared_ptr<uint8_t> data = nullptr;
                uint32_t size = 0;
                uint32_t width = 0;
                uint32_t height = 0;
                uint32_t alignWidth = 0;
                uint32_t alignHeight = 0;
                acldvppPixelFormat format;
                ImageData() {}
                ImageData(std::shared_ptr<uint8_t> buf, uint32_t bufSize,
                    uint32_t x, uint32_t y, acldvppPixelFormat fmt) : data(buf), size(bufSize),
                    width(x), height(y), format(fmt) {}
            };
            ```
        - 功能说明    
            图像数据和属性
        - 数据成员说明
            | 成员名 |  说明 |
            |---|---|
            | data | 数据，格式为共享指针，默认指向nullptr |
            | size | 数据大小，默认为0 |
            | width | 图像宽，默认为0 |
            | height | 图像高，默认为0 |
            | alignWidth | 图像对齐宽，默认为0 |
            | alignHeight | 图像对齐高，默认为0 |
            | format| 图像格式，默认为PIXEL_FORMAT_YUV_SEMIPLANAR_420（YUV420SP），目前支持如下格式：<br />PIXEL_FORMAT_YUV_SEMIPLANAR_420（YUV420SP） <br />PIXEL_FORMAT_YVU_SEMIPLANAR_420（YVU420SP)<br />PIXEL_FORMAT_YUYV_PACKED_422（YUV422) |

        - 构造方式
            ```
            // image为通过ReadBinFile读入的图片数据
            uint32_t width = 1920;
            uint32_t height = 1080;
            uint32_t imageInfoSize = YUV420SP_SIZE(1920,1080);
            void* imageInfoBuf = CopyDataToDevice((image, imageInfoSize);            
            // 1. 通过构造函数
            ImageData dst(SHARED_PTR_DVPP_BUF(imageInfoBuf), imageInfoSize, width, height, PIXEL_FORMAT_YUV_SEMIPLANAR_420);

            // 2. 通过结构体构造
            ImageData dst;
            dst.data = SHARED_PTR_DVPP_BUF(imageInfoBuf);
            dst.size = imageInfoSize;
            dst.width = width;
            dst.height = height;
            dst.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            ```

    5. **struct FrameData**
        - 结构展示
            ```
            struct FrameData {
                bool isFinished = false;
                uint32_t frameId = 0;
                uint32_t size = 0;
                void* data = nullptr;
            };
            ```
        - 功能说明    
            帧数据，主要用于队列传输
        - 数据成员说明
            | 成员名 |  说明 |
            |---|---|
            | isFinished | 是否为结束帧，默认为false |
            | frameId | 帧Id，默认为0 |
            | size | 帧图像大小，默认为0 |
            | data | 帧图像数据，默认为nullptr |

    6. **enum memoryLocation**
        - 结构展示
            ```
            enum memoryLocation {
                MEMORY_NORMAL = 0,
                MEMORY_HOST,
                MEMORY_DEVICE,
                MEMORY_DVPP,
                MEMORY_INVALID_TYPE
            };
            ```
        - 功能说明    
            内存所在位置
        - 数据成员说明
            | 枚举 | 取值 | 说明 |
            |---|---|---|
            | MEMORY_NORMAL | 0| 一般内存，用new/delete申请/释放 |  |
            | MEMORY_HOST | 1 | host侧内存，用aclrtMallocHost/aclrtFreeHost申请/释放 |
            | MEMORY_DEVICE | 2 | device侧内存，用aclrtMalloc/aclrtFree申请/释放 |
            | MEMORY_DVPP | 3 | dvpp内存，用acldvppMalloc/acldvppFree申请/释放 |
            | MEMORY_INVALID_TYPE| 4 | 无效内存类型 |

    6. **enum ResizeType**
        - 结构展示
            ```
            enum ResizeType {
                RESIZE_COMMON = 0,
                RESIZE_PROPORTIONAL_UPPER_LEFT = 1,
                RESIZE_PROPORTIONAL_CENTER = 2,
            };
            ```
        - 功能说明    
            resize方式
        - 数据成员说明
            | 枚举 | 取值 | 说明 |
            |---|---|---|
            | RESIZE_COMMON | 0 | 普通方式缩放 |  |
            | RESIZE_PROPORTIONAL_UPPER_LEFT | 1 | 等比例缩放，图像位于左上方 |
            | RESIZE_PROPORTIONAL_CENTER | 2 | 等比例缩放，图像位于中心区域 |

    7. **enum StreamProperty**
        - 结构展示
            ```
            enum StreamProperty {
                FRAME_WIDTH = 1,
                FRAME_HEIGHT = 2,
                VIDEO_FPS = 3,
                OUTPUT_IMAGE_FORMAT = 4,
                RTSP_TRANSPORT = 5,
                STREAM_FORMAT = 6
            };
            ```
        - 功能说明    
            需要设置/获取的流属性，用于视频处理过程
        - 数据成员说明
            | 枚举 | 取值 | 说明 |
            |---|---|---|
            | FRAME_WIDTH | 1 | 帧宽 |  |
            | FRAME_HEIGHT | 2 | 帧高 |
            | VIDEO_FPS | 3 | 视频帧率 |
            | OUTPUT_IMAGE_FORMAT | 4 | 输出图像格式 |
            | RTSP_TRANSPORT | 5 | RTSP流格式 |
            | STREAM_FORMAT | 6 | 视频码流格式 |