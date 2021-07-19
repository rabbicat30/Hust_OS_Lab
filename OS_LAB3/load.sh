# !/bin/bash

make
insmod mydriver5.ko
cat /proc/devices | grep mydriver5
mknod /dev/mydriver5 c 238 0
chmod 777 /dev/mydriver5

