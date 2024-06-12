#!/usr/bin/env bash

set -e

RUN_WORKLOAD="source run_workload.sh"
WORKLOADS=(
    # "workloads/zipfian-a"
    # "workloads/zipfian-b"
    # "workloads/zipfian-d"
    # "workloads/zipfian-f"
    # "workloads/zipfian-read"
    # "workloads/uniform-a"
    # "workloads/uniform-b"
    # "workloads/uniform-d"
    # "workloads/uniform-f"
    "workloads/uniform-read"
    "workloads/uniform-update"
)
POLICIES=("hash" "random")

for workload in "${WORKLOADS[@]}"
do
    for policy in "${POLICIES[@]}"
    do
        $RUN_WORKLOAD "$workload" "$policy"
        sleep 5
    done
done

python3 graph.py