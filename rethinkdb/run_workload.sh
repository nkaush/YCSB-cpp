#!/usr/bin/env bash

if [[ (! $1) ]]; then
    echo "Usage: $0 <workload file>"
    exit
fi

RETHINKDB="$HOME/rethinkdb/build/release/rethinkdb"
MISS_RATE_AGGREGATOR="$HOME/rethinkdb/aggregatemissrate.py"
DRIVERPORTS=("50000" "50001" "50002")
LOCALHOST="localhost"
WORKLOAD=$1
# pids=()

echo -n > Missrate.txt

# 1. Start 3 rethink servers on localhost, with different ports
function start_cluster() {
    for index in "${!DRIVERPORTS[@]}"
    do
        if [ "$index" -eq 0 ]; then
            $RETHINKDB --directory "node-$index" --port-offset $index &
            pids+=( $! )
            echo "adding pid $!"
            sleep 1
        else
            $RETHINKDB --directory "node-$index" --port-offset $index -j "$LOCALHOST" &
            pids+=( $! )
            echo "adding pid $!"
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
rm cache-0x*

# 3. run ycsb workload load phase 
../build/ycsb -s -db rethinkdb -load -threads 30 \
    -P $WORKLOAD \
    -p rethinkdb.hosts=$LOCALHOST:28015,$LOCALHOST:28016,$LOCALHOST:28017

# 4. After load is done, kill all 3 rethink servers. WARNING, This will kill ALL rethinkdb instances, not just the ones spawned by this script. Use the pid array for more granularity.
# for pid in "${pids[@]}"
# do
#     pkill -9 "$pid"
# done
pkill -f rethinkdb

# 5. Run ~/rethinkdb/aggregatemissrate.py to retrieve miss rate
python3 $MISS_RATE_AGGREGATOR

# 6. Run analyze.py to retrieve similarity
# python3 analyze.py

# 7. Restart cluster
echo -n > Missrate.txt
start_cluster
./usertable.py drop
./usertable.py create

# 8. Run ycsb workload run phase, repeat steps 4-6 for the run phase
../build/ycsb -s -db rethinkdb -run -threads 30 \
    -P $WORKLOAD \
    -p rethinkdb.hosts=$LOCALHOST:28015,$LOCALHOST:28016,$LOCALHOST:28017

pkill -f rethinkdb
python3 $MISS_RATE_AGGREGATOR
# python3 analyze.py