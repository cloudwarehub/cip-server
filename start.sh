#!/bin/bash
nohup Xorg -noreset -logfile /root/0.log -config /root/xorg.conf :0 &
sleep 1;
xhost +
nuhup /root/cip-server &
websockify 5998 0.0.0.0:5999