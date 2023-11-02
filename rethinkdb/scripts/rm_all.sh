#!/usr/bin/env bash

for i in $(seq 0 2); do
    ssh rethink$i -t "echo rm -r cache-* && rm -r cache-*"
done