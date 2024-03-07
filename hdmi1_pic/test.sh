#!/bin/bash

./sample_hdmi1 ut_1920x1080_nv12.yuv 10 &
../audio/sample_audio play 4 ../audio/qzgy_48k_16_mono_30s.pcm 
