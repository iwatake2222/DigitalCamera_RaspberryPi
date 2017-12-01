#!/bin/sh
CURRENT_DIR=$(cd $(dirname $0);pwd)

sleep 60s
echo `date` > $CURRENT_DIR/log_run.txt 
cd $CURRENT_DIR
sudo modprobe bcm2835-v4l2
sleep 1s
./a.out > log_exe.txt 
