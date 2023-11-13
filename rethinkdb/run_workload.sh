#!/usr/bin/env bash

echo -n > Missrate.txt

if [[ (! $1) ]]; then
    echo "Usage: $0 <workload file>"
    exit
fi

RETHINKDB="$HOME/rethinkdb/build/release/rethinkdb"
MISS_RATE_AGGREGATOR="aggregatemissrate.py"
export DRIVERPORTS=("50000" "50001" "50002")
export LOCALHOST="localhost"
export WORKLOAD=$1
export COUNTER_FILE="dumps/counter.txt"

RUN_COUNT=cat $COUNTER_FILE
export RUN_COUNT=$(($RUN_COUNT+1))
echo $RUN_COUNT > $COUNTER_FILE

LOAD_DIR="dumps/load-$RUN_COUNT"
RUN_DIR="dumps/run-$RUN_COUNT"

if [ ! -d "$LOAD_DIR" ]; then
    mkdir "$LOAD_DIR"
fi

if [ ! -d "$RUN_DIR" ]; then
    mkdir "$RUN_DIR"
fi

# 1. Start 3 rethink servers on localhost, with different ports
function start_cluster() {
    for index in "${!DRIVERPORTS[@]}"
    do
        if [ "$index" -eq 0 ]; then
            $RETHINKDB --directory "node-$index" --port-offset $index --cache-size 150 --bind all &
            sleep 1
        else
            $RETHINKDB --directory "node-$index" --port-offset $index --cache-size 150 -j "$LOCALHOST" --bind all &
            sleep 1
        fi
    done
}

start_cluster
sleep 5

# 2. call usertable drop then create
./usertable.py drop
./usertable.py create

echo -n > Missrate.txt

# 3. run ycsb workload load phase 
../build/ycsb -s -db rethinkdb -load -threads 30 \
    -P $WORKLOAD \
    -p rethinkdb.hosts=$LOCALHOST:28015,$LOCALHOST:28016,$LOCALHOST:28017 \

# 4. After load is done, kill all 3 rethink servers. WARNING, This will kill ALL rethinkdb instances, not just the ones spawned by this script. Use the pid array for more granularity.
pkill -f rethinkdb
echo "Killed rethink"

# 5. Run ~/rethinkdb/aggregatemissrate.py to retrieve miss rate
sleep 10
echo "AGGREGATING AFTER LOAD"
python3 $MISS_RATE_AGGREGATOR -p load
mv cache-0x* dumps/load-$RUN_COUNT
# 6. Run analyze.py to retrieve similarity
# python3 analyze.py
sleep 10

# 7. Restart cluster
echo -n > Missrate.txt
echo "STARTING NEW CLUSTER"
start_cluster

sleep 10

# 8. Run ycsb workload run phase, repeat steps 4-6 for the run phase. NO NEED ANYMORE, OG COMMAND RUNS BOTH
../build/ycsb -s -db rethinkdb -t -threads 30 \
    -P $WORKLOAD \
    -p rethinkdb.hosts=$LOCALHOST:28015,$LOCALHOST:28016,$LOCALHOST:28017 

sleep 10

pkill -f rethinkdb

sleep 10

python3 $MISS_RATE_AGGREGATOR -p run -f "dumps/mrate-$RUN_COUNT"
mv cache-0x* dumps/run-$RUN_COUNT || true
# python3 analyze.py
rm -rf node-*/*