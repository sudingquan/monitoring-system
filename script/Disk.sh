#!/bin/bash
time=$(date "+%Y-%m-%d__%H:%M:%S")
disk=$(df -m | grep "^/dev" | awk -v time=${time} '{printf("%s 1 %s\t%dM\t%dM\t%s\n", time, $6, $2, $4, $5);a+=$2;b+=$4}END{printf("%s 0 disk\t%dM\t%dM\t%d%%\n", time, a, b, (a-b)/a*100)}')
echo "${disk}"
