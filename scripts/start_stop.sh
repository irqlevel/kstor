#!/bin/bash
for i in {1..1}
do
    rm -f lfile10
    dd if=/dev/zero of=lfile10 bs=1M count=100
    sync
    sync
    losetup -d /dev/loop10
    losetup /dev/loop10 lfile10
    insmod bin/kstor.ko
    sleep 5
    rmmod kstor
    losetup -d /dev/loop10
    rm -f lfile10
    echo "$i times"
done
