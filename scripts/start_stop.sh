#!/bin/bash
for i in {1..100}
do
    insmod bin/kcpp.ko
    rmmod kcpp
    echo "$i times"
done
