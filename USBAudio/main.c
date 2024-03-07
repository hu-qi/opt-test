#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int main(int argc, char** argv)
{
    uint32_t argNum = 2;
    if (argc < argNum) {
        printf("Please input: ./main plughw:[card]\n");
        return -1;
    }

    int ret = 0;
    char errors[1024];
    int count = 0;
    AVPacket pkt;//数据存放
    char *devicename = argv[1];//mac： :0
    AVFormatContext *fmt_ctx = NULL;  //记得赋值NULL 上下文
    AVDictionary *options = NULL;
    char *out = "./audio.pcm";
    FILE *outfile = NULL;
    char input_command[128];
    int flag=-1;

    avdevice_register_all();  //打开所有设备
    AVInputFormat *iformat = av_find_input_format("alsa");//设置平台格式  mac: avfoundation 
    if( (ret = avformat_open_input(&fmt_ctx, devicename,iformat,&options) )< 0)//传入参数 打开设备
    {
       av_strerror(ret,errors,1024);
       av_log(NULL,AV_LOG_DEBUG,"Failed to open audio device,[%d]%s\n",ret,errors);
       return -1;
    }
    av_init_packet(&pkt);//数据初始化 干净的空间；
   
   //create file
    outfile = fopen(out,"wb+");
    flag=fcntl(0,F_GETFL); //获取当前flag
    flag |=O_NONBLOCK; //设置新falg
    fcntl(0,F_SETFL,flag); //更新flag

    printf("Start record!\n");

    while((ret = av_read_frame(fmt_ctx,&pkt)) == 0)
    {
        //write FILE
        fwrite(pkt.data,pkt.size,1,outfile);
        fflush(outfile); 
        if((ret=read(0,input_command,sizeof(input_command))) > 0)
        {
            if(strncmp(input_command, "over",4) == 0)
            {
                av_log(NULL,AV_LOG_DEBUG,"over\n");
                break;
            }
            else
            {
                av_log(NULL,AV_LOG_DEBUG,"请重新输入\n");
            }
            memset(input_command, 0, sizeof(input_command));
        }
       av_log(NULL,AV_LOG_DEBUG,"pkt_size:%d(%p)\n",pkt.size,pkt.data);
       av_packet_unref(&pkt);//缓冲区  内存释放
    }

    fclose(outfile);
    avformat_close_input(&fmt_ctx);
    
    av_log(NULL,AV_LOG_DEBUG,"Finish\n");
    return 0;
}
