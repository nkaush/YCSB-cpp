# replicated-splinterdb YCSB Binding

## Building:

```bash
export CC=clang
export LD=clang
export CXX=clang++
git clone https://github.com/nkaush/YCSB-cpp.git
cd YCSB-cpp && git branch -v -a && git checkout replicated-splinterdb
mkdir build && cd build
git submodule update --init --recursive
sudo ../replicated-splinterdb/replicated-splinterdb/setup_server.sh
cmake -DBIND_SPLINTERDB=ON .. && make -j `nproc` \
    && make -j `nproc` spl-client \
    && mv ./replicated-splinterdb/replicated-splinterdb/apps/spl-client .
```
