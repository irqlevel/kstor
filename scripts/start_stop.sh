#!/bin/bash
for i in {1..1}
do
    insmod bin/kstorage.ko
    sleep 1
    rmmod kstorage
    echo "$i times"
done
