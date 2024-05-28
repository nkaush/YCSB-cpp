#!/usr/bin/env bash

set -e

echo -n > Missrate.txt
echo -n > readmiss.txt
echo -n > writemiss.txt

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <workload file> <random | roundrobin | hash>"
    return
fi

RETHINKDB="$HOME/rethinkdb/build/release/rethinkdb"
MISS_RATE_AGGREGATOR="aggregatemissrate.py"
export LOCALHOST="localhost"
export WORKLOAD=$1
length=${#WORKLOAD}
export WORKLOAD_NUM=$(basename $WORKLOAD)
export READ_POLICY=$2

FIELDLENGTH=4096

if [ "$READ_POLICY" != "random" ] && [ "$READ_POLICY" != "roundrobin" ] && [ "$READ_POLICY" != "hash" ]; then
    echo "Usage: $0 <workload file> <random | roundrobin | hash>"
    return
fi

LOAD_DIR="dumps/load-$WORKLOAD_NUM-$READ_POLICY"
RUN_DIR="dumps/run-$WORKLOAD_NUM-$READ_POLICY"

CACHED_RUN=0
CACHED_LOAD=0

if [ ! -d "$LOAD_DIR" ]; then
    mkdir "$LOAD_DIR"
else
    CACHED_LOAD=1
fi

if [ ! -d "$RUN_DIR" ]; then
    mkdir "$RUN_DIR"
else
    CACHED_RUN=1
fi

# 1. Start 3 rethink servers on localhost, with different ports
function start_cluster() {
    for index in {0..2};
    do
        if [ ! -d "node-$index" ]; then
            mkdir "node-$index"
        fi

        if [ "$index" -eq 0 ]; then
            $RETHINKDB --directory "node-$index" --port-offset $index --cache-size 166 --bind all &
            sleep 1
        else
            $RETHINKDB --directory "node-$index" --port-offset $index --cache-size 166 -j "$LOCALHOST" --bind all &
            sleep 1
        fi
    done
}

if [ "$CACHED_LOAD" -eq 0 ]; then
    start_cluster
    sleep 5
    # 2. call usertable drop then create
    ./usertable.py drop
    ./usertable.py create

    echo -n > Missrate.txt

    # 3. run ycsb workload load phase 
    ../build/ycsb -s -db rethinkdb -load -threads 40 \
        -P $WORKLOAD \
        -p rethinkdb.hosts=$LOCALHOST:28015,$LOCALHOST:28016,$LOCALHOST:28017 \
        -p rethinkdb.read_policy=$READ_POLICY \
        -p fieldlength=$FIELDLENGTH

    # 4. After load is done, kill all 3 rethink servers. WARNING, This will kill ALL rethinkdb instances, not just the ones spawned by this script. Use the pid array for more granularity.
    pkill -f rethinkdb
    echo "Killed rethink"

    # 5. Run ~/rethinkdb/aggregatemissrate.py to retrieve miss rate
    sleep 10
    echo "AGGREGATING AFTER LOAD"
    mv cache-* $LOAD_DIR
    cp Missrate.txt "dumps/totalmisses-$WORKLOAD_NUM-$READ_POLICY-load"
    python3 $MISS_RATE_AGGREGATOR -p load -f "dumps/mrate-$WORKLOAD_NUM-$READ_POLICY" -w $WORKLOAD -rp $READ_POLICY
    # python3 tmp.py
    sleep 10
else
    echo "Found cached load files. Continuing without load phase"
fi

if [ "$CACHED_RUN" -eq 0 ]; then
    # 7. Restart cluster
    echo -n > Missrate.txt
    echo "STARTING NEW CLUSTER"
    start_cluster

    sleep 10
    # 8. Run ycsb workload run phase, repeat steps 4-6 for the run phase. NO NEED ANYMORE, OG COMMAND RUNS BOTH
    ../build/ycsb -s -db rethinkdb -t -threads 40 \
        -P $WORKLOAD \
        -p rethinkdb.hosts=$LOCALHOST:28015,$LOCALHOST:28016,$LOCALHOST:28017  \
        -p rethinkdb.read_policy=$READ_POLICY \
        -p fieldlength=$FIELDLENGTH

    pkill -f rethinkdb

    sleep 10

    cp Missrate.txt "dumps/totalmisses-$WORKLOAD_NUM-$READ_POLICY-run"
    mv cache-* $RUN_DIR || true
    python3 $MISS_RATE_AGGREGATOR -p run -f "dumps/mrate-$WORKLOAD_NUM-$READ_POLICY" -w $WORKLOAD -rp $READ_POLICY
    python3 analyze.py -p run -i $WORKLOAD_NUM -rp $READ_POLICY
    rm -rf node-*/*
else
    echo "Found cached run files. Continuing without load phase"
fi

# python3 tmp.py -w $WORKLOAD -p run -rp $READ_POLICY