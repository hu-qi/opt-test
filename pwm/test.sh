#!/bin/bash

#设置PWM3管脚复用关系为pad_pwm3。
devmem 0x00C40000D0 w 0

#设置PWM风扇测量时间窗大小为250ms。
#标准时间窗测量大小1S对应8000周期，250ms对应2000周期。
devmem 0x00C4080108 w 2000

#设置PWM脉冲周期为10000。
devmem 0x00C408002C w 10000

#设置PWM低电平开始时刻。
devmem 0x00C4080030 w 0

#设置PWM高电平开始时刻，即可设置PWM占空比（风扇转速百分比），此处以PWM占空比为50%举例。
devmem 0x00C4080034 w 5000

#查询PWM占空比。
#devmem 0x00C4080034
