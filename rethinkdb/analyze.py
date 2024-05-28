#!/usr/bin/env python3
from typing import Any, Dict, List
from functools import reduce

import sys
import os
import re
import argparse

def parse_args(args_list: List[str]):
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--phase", type=str, default="load")
    parser.add_argument("-i", "--workload", type=str, default="none")
    parser.add_argument("-rp", "--readpolicy", type=str, default="none")
    parser.add_argument("-s", "--cachesize", type=int, default=250)

    return parser.parse_args(args_list)


def find_union(keysets: List[set[Any]]) -> set[Any]:
    return reduce(lambda x, y: x | y, keysets)


def find_intersection(keysets: List[set[Any]]) -> set[Any]:
    return reduce(lambda x, y: x & y, keysets)


def find_similarity_union(keysets: List[set[str]]) -> float:
    """
    similarity = (N / N-1) * (1 - (size of union / total # of elements))
    """
    union = find_union(keysets)
    size = sum(len(s) for s in keysets)
    count = len(keysets)
    similarity = (count / (count - 1)) * (1 - (len(union) / size))

    del union

    return similarity


def find_similarity_intersection(keysets: List[set[str]]) -> float:
    """
    similarity = (size of intersection / avg size)
    """
    intersection = find_intersection(keysets)
    avg_size = sum(len(s) for s in keysets) / len(keysets)
    similarity = len(intersection) / avg_size

    del intersection

    return similarity


if __name__ == "__main__":
    args = parse_args(sys.argv[1:])
    phase = args.phase
    workload = args.workload
    read_policy = args.readpolicy
    cache_size = args.cachesize
    if (phase != "load" and phase != "run") or workload == "none" or read_policy == "none":
        print("Usage: ./analyze.py -p <load | run> -i <workload> -rp <random | roundrobin | hash>")
        exit(1)

    dumpfile: str
    if phase == "load":
        dumpfile = f"dumps/load-{workload}-{read_policy}"
    elif phase == "run":
        dumpfile = f"dumps/run-{workload}-{read_policy}"
    else:
        print("ERROR: Phase provided to analyze.py was not 'run' or 'load'")
        exit()

    cachecontents: Dict[str, List] = {}
    for filename in os.listdir(dumpfile):
        # Check if the current item is a file
        abs_path = os.path.join(dumpfile, filename)
        if os.path.isfile(abs_path):
            _, pid, _ = filename.split("-")
            with open(abs_path, "rb") as f:
                if pid in cachecontents:
                    cachecontents[pid].append(f.read())
                else:
                    cachecontents[pid] = [f.read()]

    regex = b"id\x06\x18user\d{20}"
    cachecontents: Dict[str, bytes] = {p: b"".join(l) for p, l in cachecontents.items()}
    regex_matches = [re.findall(regex, cc) for cc in cachecontents.values()]
    keysets = [set(m[4:].decode("utf-8") for m in match) for match in regex_matches]
    keys_total = sum(len(s) for s in keysets)

    # for key in caches:
    with open(f"dumps/similarity-{workload}-{read_policy}", "w", encoding="utf8") as f:
        similarity = find_similarity_union(keysets)
        print(f"Similarity for {phase} phase on workload {workload} was {similarity}, with {keys_total} keys in cache")
        f.write(str(similarity))
    
    with open(f"dumps/keys_cached-{workload}-{read_policy}-{cache_size}", "w", encoding="utf8") as f:
        f.write(str(keys_total))
