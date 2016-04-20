#!/bin/bash
nohup Xorg -noreset -logfile /root/0.log -config /root/xorg.conf :0 &
sleep 1;
xhost +
/root/cip-server