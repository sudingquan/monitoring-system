#!/bin/bash
for i in {2..10}; do
    scppi ../script/ sudingquan@pi${i}:~
    scppi ./client sudingquan@pi${i}:~/client
    
    scppi ./client_conf sudingquan@pi${i}:~/client
done;
