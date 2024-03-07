#!/bin/bash

audio_file=/tmp/test.pcm

./sample_audio capture $audio_file
./sample_audio play 2 $audio_file
rm $audio_file
