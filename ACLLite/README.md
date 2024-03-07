# ACLLite

## 介绍
ACLLite库是对CANN提供的ACL接口进行的高阶封装，简化用户调用流程，为用户提供一组简易的公共接口。当前主要针对边缘场景设计。

## 软件架构
<table>
<tr><td width="25%"><b>命名空间</b></td><td width="25%"><b>模块</b></td><td width="50%"><b>说明</b></td></tr>
<tr><td rowspan="10">acllite</td>
<td>common</td>  <td>资源管理及公共函数模块</td>  </tr>
<tr><td>DVPPLite</td>  <td>DVPP高阶封装模块</td>  </tr>
<tr><td>OMExecute</td>  <td>离线模型执行高阶封装模块</td>  </tr>
<tr><td>Media</td>  <td>媒体功能高阶封装模块</td>  </tr>
</tr>
</table>



## 安装教程

- **环境要求：** Ascend边缘设备。   
- **CANN版本要求：** 7.0及以上社区版本。 
- **安装步骤：**   
    ```
    # 拉取ACLLite仓库，并进入目录
    git clone https://gitee.com/ascend/ACLLite.git
    cd ACLLite

    # 安装ffmpeg
    apt-get install libavcodec-dev libswscale-dev libavdevice-dev

    # 设置环境变量，其中DDK_PATH中/usr/local请替换为实际CANN包的安装路径
    export DDK_PATH=/usr/local/Ascend/ascend-toolkit/latest
    export NPU_HOST_LIB=$DDK_PATH/runtime/lib64/stub

    # 安装，编译过程中会将库文件安装到/lib目录下，所以会有sudo命令，需要输入密码
    bash build_so.sh
    ```  

## API说明

- [common模块API说明](Doc/common.md)
- [DVPPLite模块API说明](Doc/dvpplite.md)
- [OMExcute模块API说明](Doc/omexcute.md)
- [Media模块API说明](Doc/media.md)

#### 参与贡献

1.  Fork 本仓库
2.  提交代码
3.  新建 Pull Request


#### 修订记录

| 日期  | 更新事项  |
|---|---|
| 2022/1/25  | 首次发布  |

