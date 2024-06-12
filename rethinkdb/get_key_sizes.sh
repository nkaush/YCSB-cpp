#!/usr/bin/env bash

echo -n > Missrate.txt

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <workload file> <random | roundrobin | hash> <cache size in MB>"
    return
fi

RETHINKDB="$HOME/rethinkdb/build/release/rethinkdb"
export LOCALHOST="localhost"
export WORKLOAD=$1
length=${#WORKLOAD}
export WORKLOAD_NUM=${WORKLOAD:length-1:1}
export READ_POLICY=$2
CACHE_SIZE=$3

if [ "$READ_POLICY" != "random" ] && [ "$READ_POLICY" != "roundrobin" ] && [ "$READ_POLICY" != "hash" ]; then
    echo "Usage: $0 <workload file> <random | roundrobin | hash>"
    return
fi

LOAD_DIR="dumps/load-$WORKLOAD_NUM-$READ_POLICY"

if [ ! -d "$LOAD_DIR" ]; then
    mkdir "$LOAD_DIR"
fi

# 1. Start 3 rethink servers on localhost, with different ports
function start_cluster() {
    for index in {0..2};
    do
        if [ ! -d "node-$index" ]; then
            mkdir "node-$index"
        fi

        if [ "$index" -eq 0 ]; then
            $RETHINKDB --directory "node-$index" --port-offset $index --cache-size $CACHE_SIZE --bind all &
            sleep 1
        else
            $RETHINKDB --directory "node-$index" --port-offset $index --cache-size $CACHE_SIZE -j "$LOCALHOST" --bind all &
            sleep 1
        fi
    done
}

start_cluster
sleep 5

./usertable.py drop
./usertable.py create

echo -n > Missrate.txt

# 3. run ycsb workload load phase 
../build/ycsb -s -db rethinkdb -load -threads 50 \
    -P $WORKLOAD \
    -p rethinkdb.hosts=$LOCALHOST:28015,$LOCALHOST:28016,$LOCALHOST:28017 \
    -p rethinkdb.read_policy=$READ_POLICY

# 4. After load is done, kill all 3 rethink servers. WARNING, This will kill ALL rethinkdb instances, not just the ones spawned by this script. Use the pid array for more granularity.
pkill -f rethinkdb
echo "Killed rethink"

sleep 10
mv cache-0x* $LOAD_DIR
cp Missrate.txt "dumps/totalmisses-$WORKLOAD_NUM-$READ_POLICY-load"
python3 analyze.py -p load -i $WORKLOAD_NUM -rp $READ_POLICY -s $CACHE_SIZE
rm -rf node-*/*