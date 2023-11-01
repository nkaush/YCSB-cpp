#!/usr/bin/env bash

TS=$(date '+%Y%m%d%H%M%S')

echo $TS

for i in $(seq 0 2); do
    mkdir -p dumps/rethink$i
    scp rethink$i:rethinkdb/cache-* dumps/rethink$i
    cat dumps/rethink$i/cache-* > "dumps/rethink$i/cache.$TS"
    rm dumps/rethink$i/cache-*
done