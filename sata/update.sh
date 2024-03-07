#!/bin/bash

rootpart=$(findmnt -n -o SOURCE / | sed 's~\[.*\]~~')
rootdevice=$(lsblk -n -o PKNAME $rootpart | head -1)
dev_name="/dev/""$rootdevice"

dd if=../dt_img/dt_drm_sata.img of=${dev_name} count=4096 seek=114688 bs=512

sync
reboot
