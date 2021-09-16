#!/usr/bin/env bash
cd /sys/class/remoteproc/remoteproc1
echo 'ad677d.out' > firmware
echo 'start' > state


