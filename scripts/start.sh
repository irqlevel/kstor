#!/bin/bash -xv

WDIR=temp
LOOP_NAME=loop21
LOOP_FILE=loop21-file

rm -rf $WDIR
mkdir -p $WDIR

dd if=/dev/zero of=$WDIR/$LOOP_FILE bs=1M count=100

losetup -d /dev/$LOOP_NAME
losetup /dev/$LOOP_NAME $WDIR/$LOOP_FILE

modprobe dns-resolver
insmod bin/kstor.ko

echo 0 > /sys/kernel/debug/tracing/tracing_on
echo 'nop' > /sys/kernel/debug/tracing/current_tracer
echo 100000 > /sys/kernel/debug/tracing/buffer_size_kb
echo '' > /sys/kernel/debug/tracing/trace
echo 1 > /sys/kernel/debug/tracing/events/kstor/enable
echo 1 > /sys/kernel/debug/tracing/tracing_on

bin/kstor-ctl mount /dev/$LOOP_NAME 4096
bin/kstor-ctl start-server 127.0.0.1 8111
