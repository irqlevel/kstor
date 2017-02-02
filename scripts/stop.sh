#!/bin/bash
WDIR=temp
LOOP_NAME=loop21

bin/kstor-ctl stop-server
bin/kstor-ctl umount /dev/$LOOP_NAME

echo 0 > /sys/kernel/debug/tracing/tracing_on
cat /sys/kernel/debug/tracing/trace > $WDIR/trace
echo '' > /sys/kernel/debug/tracing/trace
echo 0 > /sys/kernel/debug/tracing/events/kstor/enable

rmmod kstor

losetup -d /dev/$LOOP_NAME
