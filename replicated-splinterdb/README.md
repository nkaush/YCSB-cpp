# replicated-splinterdb YCSB Binding

## Sample Usage:

```bash
docker run -it --rm --network splinterdb-network neilk3/ycsb-replicated-splinterdb -s -load -t -db replicated-splinterdb -P workloada -p replicated_splinterdb.host=replicated-splinterdb-node-1
```

## Sample CLI Client
```bash
./replicated-splinterdb/client.sh 1 -e ls
```