#!/bin/bash
for i in {1..100}
do
    insmod bin/kcpp.ko
    sleep 1
    rmmod kcpp
    echo "$i times"
done
