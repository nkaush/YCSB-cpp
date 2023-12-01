# replicated-splinterdb YCSB Binding

## Building:

```bash
export CC=clang
export LD=clang
export CXX=clang++
git clone https://github.com/nkaush/YCSB-cpp.git
cd YCSB-cpp && mkdir build && cd build
git submodule update --init --recursive
sudo ../replicated-splinterdb/replicated-splinterdb/setup_server.sh
cmake -DBIND_SPLINTERDB=ON .. && make -j `nproc` \
    && make -j `nproc` spl-client \
    && mv ./replicated-splinterdb/replicated-splinterdb/apps/spl-client .
```

## Sample Usage:

```bash
docker run -it --rm --network splinterdb-network neilk3/ycsb-replicated-splinterdb -s -load -db replicated-splinterdb -p replicated_splinterdb.host=replicated-splinterdb-node-1 -threads 10 -P 700k 
```

## Sample CLI Client
```bash
./replicated-splinterdb/client.sh 1 -e ls
```
