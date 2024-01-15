#!/bin/bash

## Check if we have root permissions
if [ "`id -u`" != "0" ]; then
        echo "ERROR: Need to be root to run this script! Use 'sudo' command."
        exit
fi

echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
echo performance > /sys/devices/system/cpu/cpufreq/policy1/scaling_governor
echo performance > /sys/devices/system/cpu/cpufreq/policy2/scaling_governor
echo performance > /sys/devices/system/cpu/cpufreq/policy3/scaling_governor
## ./main  pairs  period_usec  duration_sec
./main-std 5 10000 10 # 10 ms
echo powersave   > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
echo powersave   > /sys/devices/system/cpu/cpufreq/policy1/scaling_governor
echo powersave   > /sys/devices/system/cpu/cpufreq/policy2/scaling_governor
echo powersave   > /sys/devices/system/cpu/cpufreq/policy3/scaling_governor
