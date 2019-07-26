#!/bin/bash
for i in {2..10}; do
    scppi ./client sudingquan@pi${i}:~
    scppi ./client_conf sudingquan@pi${i}:~
done;
