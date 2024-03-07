#!/bin/bash

npu-smi set -t pwm-mode -d 0 > /dev/null
npu-smi set -t pwm-duty-ratio -d 100 > /dev/null
