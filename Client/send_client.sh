#!/bin/bash
for i in {2..10}; do
    scppi ../script/ sudingquan@pi${i}:~
    scppi ./client sudingquan@pi${i}:~
    scppi ./client_conf sudingquan@pi${i}:~
done;
