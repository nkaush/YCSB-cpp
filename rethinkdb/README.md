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