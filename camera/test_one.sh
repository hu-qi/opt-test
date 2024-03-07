#!/bin/bash

./vi_l1_sample 1 1 1 > /dev/null

#[[ -f vi_pipe0_chn0_w1920_h1080.yuv ]] && ./sample_hdmi vi_pipe0_chn0_w1920_h1080.yuv 10
#ffplay -pix_fmt yuv420p -video_size 1920*1080 ./vi_pipe0_chn0_w1920_h1080.yuv
