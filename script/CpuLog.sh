#!/bin/bash
time=$(date "+%Y-%m-%d__%H:%M:%S");
min1=$(cat /proc/loadavg | cut -d " " -f 1);
min5=$(cat /proc/loadavg | cut -d " " -f 2);
min15=$(cat /proc/loadavg | cut -d " " -f 3);
t1=0;
t2=0;
idle1=`cat /proc/stat | head -1 | cut -d " " -f 6`;
stat1=`cat /proc/stat | head -1 | awk '{$1="";print $0}'`;
sleep 0.5;
idle2=`cat /proc/stat | head -1 | cut -d " " -f 6`
stat2=`cat /proc/stat | head -1 | awk '{$1="";print $0}'`;

for i in ${stat1}; do
    t1=$[ ${t1} + ${i} ];
done
for i in ${stat2}; do
    t2=$[ ${t2} + ${i} ];
done
idle=$[ ${idle2} - ${idle1} ];
totalCpuTime=$[ ${t2} - ${t1} ];
usage=$(awk -v x=${idle} -v y=${totalCpuTime} 'BEGIN {printf "%.2f\n",100*(y-x)/y}')
true_temp=$(cat /sys/class/thermal/thermal_zone0/temp);
temp=$(awk -v x=${true_temp} -v y=1000 'BEGIN {printf "%.2f\n",x/y}')
if [[ ${true_temp} -lt 50000 ]]; then
    alert="nolmal";
elif [[ ${true_temp} -lt 70000 ]]; then
    alert="note";
else
    alert="warning";
fi
echo ${time} ${min1} ${min5} ${min15} ${usage}"%" ${temp}"℃" ${alert};
