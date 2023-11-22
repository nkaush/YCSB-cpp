#!/usr/bin/env bash

RUN_WORKLOAD="source run_workload.sh"
WORKLOADS=("workloads/workloadd")
POLICIES=("roundrobin" "hash" "random")

for workload in "${WORKLOADS[@]}"
do
    for policy in "${POLICIES[@]}"
    do
        $RUN_WORKLOAD "$workload" "$policy"
        sleep 10
    done
done

python3 graph.py