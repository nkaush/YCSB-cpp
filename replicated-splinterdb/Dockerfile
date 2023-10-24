FROM neilk3/splinterdb-build-env as build

RUN apt-get update -y \
    && apt-get install -y cmake zlib1g-dev libgflags-dev \
    && apt-get clean

ENV CC clang
ENV CXX clang++-13
ENV LD clang++-13

WORKDIR /build

COPY core core
COPY yaml-cpp yaml-cpp
COPY replicated-splinterdb replicated-splinterdb
COPY HdrHistogram_c HdrHistogram_c
COPY CMakeLists.txt /build

RUN cmake -DBIND_SPLINTERDB=1 . && make -j `nproc`

FROM neilk3/splinterdb-run-env as run

WORKDIR /run

COPY workloads /run/
COPY --from=build /build/ycsb /run/

ENTRYPOINT [ "/run/ycsb" ]