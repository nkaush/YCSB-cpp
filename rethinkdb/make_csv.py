#!/usr/bin/env python

from analyze import find_similarity_union, find_similarity_intersection, find_union
from os.path import exists, getsize, join, isfile
from typing import List, Dict
from pandas import DataFrame
from os import listdir
from re import findall
from glob import glob
from tqdm import tqdm

# (read frequency, RMW frequency, insert frequency)
WORKLOAD_OP_DISTRIBUTION = {
    "a": (0.5, 0.5, 0.0),
    "b": (0.95, 0.05, 0.0),
    "d": (0.95, 0.0, 0.05),
    "f": (0.5, 0.5, 0.0),
    "read": (1.0, 0.0, 0.0),
    "update": (0.0, 1.0, 0.0)
}


def compute_cluster_opcount(name, raw_opcount=500000, numreplicas=3):
    read_freq, rmw_freq, ins_freq = WORKLOAD_OP_DISTRIBUTION[name]
    
    # 1 read + (1 update per replica)
    rmw_opcount = rmw_freq * raw_opcount * (numreplicas + 1)
    ins_opcount = ins_freq * raw_opcount * numreplicas
    read_opcount = read_freq * raw_opcount
    
    return rmw_opcount + ins_opcount + read_opcount


rows = []
for path in tqdm(glob("dumps-*/similarity-*")):
    directory, filename = path.split("/")
    _, distribution, name, read_policy = filename.split("-")
    _, valuesize = directory.split("-")
    valuesize = 1024 * int(valuesize[0])

    workload_name = f"{distribution}-{name}"
    misses_path = f"{directory}/totalmisses-{workload_name}-{read_policy}-run"
    keycount_path = f"{directory}/keys_cached-{workload_name}-{read_policy}-250"
    similarity_path = f"{directory}/similarity-{workload_name}-{read_policy}"
    cache_path = f"{directory}/run-{workload_name}-{read_policy}"

    assert exists(misses_path)
    assert exists(keycount_path)
    assert exists(similarity_path)
    assert exists(cache_path)

    cachecontents: Dict[str, List] = {}
    for filename in listdir(cache_path):
        # Check if the current item is a file
        abs_path = join(cache_path, filename)
        if isfile(abs_path):
            _, pid, _ = filename.split("-")
            with open(abs_path, "rb") as f:
                if pid in cachecontents:
                    cachecontents[pid].append(f.read())
                else:
                    cachecontents[pid] = [f.read()]

    regex = b"id\x06\x18user\d{20}"
    cachecontents: Dict[str, bytes] = {p: b"".join(l) for p, l in cachecontents.items()}
    regex_matches = [findall(regex, cc) for cc in cachecontents.values()]
    keysets = [set(m[4:].decode("utf-8") for m in match) for match in regex_matches]

    similarity_intersection = find_similarity_intersection(keysets)
    similarity_union = find_similarity_union(keysets)
    cef = len(find_union(keysets)) / 100_000

    with open(similarity_path, "r") as f:
        similarity = float(f.read())
        assert similarity == similarity_union

    with open(keycount_path, "r") as f:
        keycount = int(f.read())

    with open(misses_path, "r") as f:
        misses = sum(int(l.replace("Miss rate:", "")) for l in f.readlines() if l)

    opcount = compute_cluster_opcount(name)
    miss_rate = misses / opcount
    
    mib_cached = sum(getsize(join(cache_path, f)) for f in listdir(cache_path))
    mib_cached /= (1024 * 1024 * 3)

    cache_ratio = (keycount / 100_000) / 3

    rows.append({
        "Cachesize (MiB)": mib_cached,
        "Cache Ratio": cache_ratio,
        "Value Size (Bytes)": valuesize,
        "Read Policy": read_policy,
        "Workload": workload_name,
        "CEF": cef,
        "Similarity (Union)": similarity,
        "Similarity (Intersection)": similarity_intersection,
        "Cluster Miss Rate": miss_rate
    })

df = DataFrame(rows).sort_values(by=["Value Size (Bytes)", "Read Policy", "Workload"])
df["Normalized Cluster Miss Rate"] = df["Cluster Miss Rate"] / df["Cluster Miss Rate"].max()
df.to_csv("results.csv", index=False)
