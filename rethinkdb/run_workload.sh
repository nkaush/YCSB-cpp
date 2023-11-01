#!/usr/bin/env bash

if [[ (! $1) ]]; then
    echo "Usage: $0 <workload file>"
fi

./usertable.py drop
./usertable.py create

../build/ycsb -s -db rethinkdb -load -run -threads 30 \
    -P workloads/$1 \
    -p rethinkdb.read_mode=outdated \
    -p rethinkdb.durability=soft