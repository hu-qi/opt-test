#include <iostream>
#include <stdio.h>
#include <fcntl.h>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

int main(int argc, char** argv) {
    uint32_t argNum = 2;
    if (argc < argNum) {
        printf("Please input: ./main /dev/video[number]\n");
        return -1;
    }

    // 注册
    avdevice_register_all();
    
    // 打开摄像头设备
    AVFormatContext* formatContext = nullptr;
    AVDictionary *options = NULL;
    AVInputFormat *iformat = av_find_input_format("v4l2");

    av_dict_set(&options,"video_size","1280*720",0);
    av_dict_set(&options,"framerate","30",0);
    av_dict_set(&options,"pixel_format","yuv420p",0);

    if (int err_code = avformat_open_input(&formatContext, argv[1], iformat, &options)) {
        std::cout << "err_cde is: " << err_code <<std::endl;
        return -1;
    }
    
    // 查找并打开摄像头流
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cout << "无法找到摄像头流" << std::endl;
        return -1;
    }
    
    int videoStreamIndex = -1;
    
    // 寻找视频流索引
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    
    if (videoStreamIndex == -1) {
        std::cout << "无法找到视频流" << std::endl;
        return -1;
    }
    
    // 获取视频编解码器上下文
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
     // 寻找解码器
    AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        std::cout << "无法找到解码器" << std::endl;
        return -1;
    }
    
    // 创建解码器上下文
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        std::cout << "无法创建解码器上下文" << std::endl;
        return -1;
    }
    
    // 打开解码器
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cout << "无法打开解码器" << std::endl;
        return -1;
    }
    
    // 分配帧缓冲区
    AVFrame* frame = av_frame_alloc();
    
    // 分配数据包
    AVPacket packet;
    while (av_read_frame(formatContext, &packet) >= 0) {
        // 解码视频帧
        if (packet.stream_index == videoStreamIndex) {
            int response = avcodec_send_packet(codecContext, &packet);
            if (response < 0 || response == AVERROR(EAGAIN)) {
                continue;   // 忽略错误或需要更多输入的情况
            }
            while (response >= 0) {
                response = avcodec_receive_frame(codecContext, frame);
                if (response == AVERROR(EAGAIN)) {
                    break;   // 需要更多输出，继续接收帧
                } else if (response < 0) {
                    std::cout << "错误解码视频帧" << std::endl;
                    return -1;
                }
            }
        }
        // 保存一帧图像
        FILE *fp;
        fp = fopen("out.yuv","wb");
        fwrite(packet.data,1,packet.size,fp);
        fclose(fp);
        av_packet_unref(&packet);
        break;
    }
    
    // 清理资源
    av_frame_free(&frame);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    return 0;
}
