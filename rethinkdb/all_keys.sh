#!/usr/bin/env bash

SIZES=(50 100 150 200 250)
KEYRUNNER=./get_key_sizes.sh
WORKLOAD=workloads/workloada
length=${#WORKLOAD}
WORKLOAD_NUM=${WORKLOAD:length-1:1}
POLICY=roundrobin

for size in "${SIZES[@]}"
do
    $KEYRUNNER $WORKLOAD $POLICY "$size"
    rm -rf dumps/load-$WORKLOAD_NUM-$POLICY/*
done

python3 graph_sizes.py