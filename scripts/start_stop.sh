#!/bin/bash -xv

WDIR=temp
LOOP_NAME=loop11
LOOP_FILE=loop10-file

rm -rf $WDIR
mkdir -p $WDIR

dd if=/dev/zero of=$WDIR/$LOOP_FILE bs=1M count=100

losetup -d /dev/$LOOP_NAME
losetup /dev/$LOOP_NAME $WDIR/$LOOP_FILE

insmod bin/kstor.ko

echo 0 > /sys/kernel/debug/tracing/tracing_on
echo 'nop' > /sys/kernel/debug/tracing/current_tracer
echo 100000 > /sys/kernel/debug/tracing/buffer_size_kb
echo '' > /sys/kernel/debug/tracing/trace
echo 1 > /sys/kernel/debug/tracing/events/kstor/enable
echo 1 > /sys/kernel/debug/tracing/tracing_on

bin/kstor-ctl device-add /dev/$LOOP_NAME
bin/kstor-ctl device-remove /dev/$LOOP_NAME

echo 0 > /sys/kernel/debug/tracing/tracing_on
cat /sys/kernel/debug/tracing/trace > $WDIR/trace
echo '' > /sys/kernel/debug/tracing/trace
echo 0 > /sys/kernel/debug/tracing/events/kstor/enable

rmmod kstor

losetup -d /dev/$LOOP_NAME
