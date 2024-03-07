#!/bin/bash

rootpart=$(findmnt -n -o SOURCE / | sed 's~\[.*\]~~')
rootdevice=$(lsblk -n -o PKNAME $rootpart | head -1)
dev_name="/dev/""$rootdevice"
dt_name=dt_vdp_pcie.img

if [[ $rootpart == *nvme* ]]; then
	dt_name=dt_vdp_pcie.img
elif [[ $rootpart == *sd* ]]; then
	dt_name=dt_vdp_sata.img
fi

dd if=../dt_img/${dt_name} of=${dev_name} count=4096 seek=114688 bs=512

sync
reboot
