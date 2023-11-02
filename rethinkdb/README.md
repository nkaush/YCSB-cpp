# RethinkDB YCSB Binding

## Building

```bash
export CXX=clang++
export LD=clang
cd YCSB-cpp
git submodule update --init --recursive
mkdir build && cd build
cmake -DBIND_RETHINKDB=ON ..
make -j 10
```

## Starting a Cluster

### Start the Seed Node

```bash
# Usually run this on node0
rethinkdb --bind $(hostname -I | cut -d' ' -f2) --cache-size 150
```

### Start Other Nodes in the Cluster and Join Via the Seed
```bash
# The --join IP address assumes that you initialized the seed on node0
rethinkdb --bind $(hostname -I | cut -d' ' -f2) --cache-size 150 --join 10.10.1.1:29015
```