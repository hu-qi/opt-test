#!/bin/bash

i=1
while ((i <= 100))
do
	npu-smi set -t pwm-mode -d 0 > /dev/null
	npu-smi set -t pwm-duty-ratio -d 100 > /dev/null
	sleep 5
	npu-smi set -t pwm-duty-ratio -d 0 > /dev/null
	sleep 3
done

