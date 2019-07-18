#!/bin/bash
time=$(date "+%Y-%m-%d__%H:%M:%S")
hostname=$(hostname)
os=$(cat /etc/issue | cut -d " " -f 1-3 | tr " " "_")
kernel=$(cat /proc/version | cut -d " " -f 3)
runtime=$(uptime | tr ":" " " | awk '{printf("%s_%s_%s_%s_hours,_%s_minustes %s %s %s", $4, $5, $6, $1, $2, $13, $14, $15)}')
disk=($(df -m | grep "^/dev" | awk '{a+=$2;b+=$4}END{printf("%d %d", a, (a-b)/a*100)}'))
mem=($(free -m | head -2 | tail -1 | awk '{printf("%d %d", $2, $3/$2)}'))
true_temp=$(cat /sys/class/thermal/thermal_zone0/temp)
temp=$(awk -v x=${true_temp} -v y=1000 'BEGIN {printf "%.2f\n",x/y}')
if [[ ${disk[1]} -lt 80 ]]; then
    disk_alert="normal";
elif [[ ${disk[1]} -lt 90 ]]; then
    disk_alert="note"
else
    disk_alert="warning"
fi

if [[ ${mem[1]} -lt 70 ]]; then
    mem_alert="normal";
elif [[ ${mem[1]} -lt 80 ]]; then
    mem_alert="note"
else
    mem_alert="warning"
fi

if [[ ${true_temp} -lt 50000 ]]; then
    cpu_alert="normal";
elif [[ ${true_temp} -lt 70000 ]]; then
    cpu_alert="note"
else
    cpu_alert="warning"
fi
echo "${time} ${hostname} ${os} ${kernel} ${runtime} ${disk[0]} ${disk[1]}% ${mem[0]} ${mem[1]}% ${temp} ${disk_alert} ${mem_alert} ${cpu_alert}"
