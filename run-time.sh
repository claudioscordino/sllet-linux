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
## ./main-time  pairs  send_period_usec  recv_period_usec  max_exec_time_usec  interconnect_task_usec  duration_sec
./main-time 4 10000 10000 5000 20000 30 
echo powersave   > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
echo powersave   > /sys/devices/system/cpu/cpufreq/policy1/scaling_governor
echo powersave   > /sys/devices/system/cpu/cpufreq/policy2/scaling_governor
echo powersave   > /sys/devices/system/cpu/cpufreq/policy3/scaling_governor
