#!/bin/bash
time=$(date "+%Y-%m-%d__%H:%M:%S")
pid=(`ps auxh | awk '$3 >= 50 || $4 >= 50{printf("%s ", $2)}'`)
if [[  ${#pid[@]} -gt 0 ]];then 
    sleep 5
    for i in ${pid[@]}; do
        ps aux | grep ${i} | grep -v grep | awk -v time=${time} '$3 >= 50 || $4 >= 50{printf("%s %s %s %s %s%% %s%%\n", time, $11, $2, $1, $3, $4)}'
    done
fi
