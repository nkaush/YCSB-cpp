#!/usr/bin/env bash

RUN_WORKLOAD="source run_workload.sh"
WORKLOADS=("workloads/workloada" "workloads/workloadb" "workloads/workloadc" "workloads/workloadd" "workloads/workloadf")
POLICIES=("roundrobin" "hash" "random")

for workload in "${WORKLOADS[@]}"
do
    for policy in "${POLICIES[@]}"
    do
        $RUN_WORKLOAD "$workload" "$policy"
        sleep 10
    done
done