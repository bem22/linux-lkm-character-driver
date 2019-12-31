#!/bin/bash

rmmod charDeviceDriver
rm -f /dev/opsysmem

make clean
make
insmod charDeviceDriver.ko
mknod /dev/opsysmem c 246 0
