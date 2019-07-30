#!/bin/bash
time=$(date "+%Y-%m-%d__%H:%M:%S");
DyAver=$1
if [[ ${DyAver}x == "x" ]]; then
    exit 1
fi
MemValue=(`free -m | head -2 | tail -1 | awk '{printf("%d %d", $2, $3)}'`)
total=${MemValue[0]}
used=${MemValue[1]}
left=$[ ${total} - ${used} ]
MemAvaPrec=`echo "scale=1;${used}*100/${total}" | bc`
DyAver=`echo "scale=1;${DyAver}*0.3+${MemAvaPrec}*0.7" | bc`
echo "${time} ${total}M ${left}M ${MemAvaPrec}% ${DyAver}%";
echo "${DyAver}"
