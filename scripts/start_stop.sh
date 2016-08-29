#!/bin/bash
for i in {1..1}
do
    insmod bin/kstor.ko
    sleep 1
    rmmod kstor
    echo "$i times"
done
