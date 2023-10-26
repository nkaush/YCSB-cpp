# replicated-splinterdb YCSB Binding

## Sample Usage:

```bash
docker run -it --rm --network splinterdb-network neilk3/ycsb-replicated-splinterdb -s -load -db replicated-splinterdb -p replicated_splinterdb.host=replicated-splinterdb-node-1 -threads 10 -P 700k 
```

## Sample CLI Client
```bash
./replicated-splinterdb/client.sh 1 -e ls
```

## Debug

```bash
docker network create splinterdb-network
docker run --rm -it \
    --name "replicated-splinterdb-node-1" \
    --hostname "replicated-splinterdb-node-1" \
    --network splinterdb-network \
    --entrypoint /bin/bash \
    neilk3/replicated-splinterdb
```