#!/bin/bash
time=$(date "+%Y-%m-%d__%H:%M:%S")
UserNum=$(grep -v /sbin/nologin /etc/passwd | cat -b | tail -1 | awk '{printf("%d",$1)}')
ActUser=$(last -w | head -n -2 | awk '{printf("%s\n", $1)}' | uniq | head -3 | tr "\n" ",")
RootUser=$(cat /etc/group | grep sudo | cut -d ":" -f 4)
OnlineUser=$(w | tail -n +3 | awk '{printf("%s_%s_%s,", $1, $3, $2)}')

echo "${time} ${UserNum} [${ActUser%,}] [${RootUser}] [${OnlineUser%,}]"
