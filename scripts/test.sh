#!/bin/bash -xv

WDIR=temp
LOOP_NAME=loop21
LOOP_FILE=loop21-file

bin/kstor-ctl test 1
bin/kstor-ctl umount /dev/$LOOP_NAME
bin/kstor-ctl mount /dev/$LOOP_NAME
