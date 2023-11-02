#!/usr/bin/env bash

if [[ (! $1) ]]; then
    echo "Usage: $0 <workload file>"
fi

CLIENT_NODE=rethink0
WORKLOADFILE=$1
TS=$(date '+%Y%m%d%H%M%S')

SOURCE="source ./YCSB-cpp/rethinkdb/venv/bin/activate"

ssh $CLIENT_NODE -t "$SOURCE && ./YCSB-cpp/rethinkdb/usertable.py drop"
ssh $CLIENT_NODE -t "$SOURCE && ./YCSB-cpp/rethinkdb/usertable.py create"

for i in $(seq 0 2); do
    ssh rethink$i -t "echo rm -r cache-* && rm -r cache-*"
done

ssh $CLIENT_NODE -t "./YCSB-cpp/build/ycsb -s -db rethinkdb -load -run -threads 30 \
    -P ./YCSB-cpp/rethinkdb/workloads/$WORKLOADFILE \
    -p rethinkdb.read_mode=outdated \
    -p rethinkdb.durability=soft \
    -p rethinkdb.hosts=10.10.1.1,10.10.1.2,10.10.1.3"

ssh $CLIENT_NODE -t "$SOURCE && ./YCSB-cpp/rethinkdb/usertable.py drop"

for i in $(seq 0 2); do
    echo "Extracting dumps from node rethink$i..."
    mkdir -p dumps/rethink$i
    scp rethink$i:cache-* dumps/rethink$i
    cat dumps/rethink$i/cache-* > "dumps/rethink$i/$WORKLOADFILE-$TS"
    rm dumps/rethink$i/cache-*
done