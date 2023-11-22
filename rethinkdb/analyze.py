#!/usr/bin/env python3
from functools import reduce
from typing import List, Set

import glob
import sys
import os
import re
import argparse

def parse_args(args_list: List[str]):
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--phase", type=str, default="load")
    parser.add_argument("-i", "--workload", type=str, default="none")
    parser.add_argument("-rp", "--readpolicy", type=str, default="none")

    return parser.parse_args(args_list)


args = parse_args(sys.argv[1:])
phase = args.phase
workload = args.workload
read_policy = args.readpolicy
if (phase != "load" and phase != "run") or workload == "none" or read_policy == "none":
    print("Usage: ./analyze.py -p <load | run> -i <workload> -rp <random | roundrobin | hash>")
    exit(1)


def find_similarity(keysets: List[set]) -> float:
    union = reduce(lambda x, y: x | y, keysets)
    size = sum(len(s) for s in keysets)
    count = len(keysets)

    similarity = (count / (count - 1)) * (1 - (len(union) / size))
    return similarity

def find_similarity_new(keylist: List[str], num_replicas: int=3) -> float:
    union = set().union(keylist)
    count = num_replicas
    size = len(keylist)

    similarity = (count / (count - 1)) * (1 - (len(union) / size))
    return similarity

if __name__ == "__main__":
    keys_found = []

    if phase == "load":
        for loaddump in glob.glob(f"dumps/load-{workload}-{read_policy}"):

            cachecontents = []
            for filename in os.listdir(loaddump):
                # Check if the current item is a file
                abs_path = os.path.join(loaddump, filename)
                if os.path.isfile(abs_path):
                    with open(abs_path, "rb") as f:
                        cachecontents.append(f.read())

            cachecontents = b"".join(cachecontents)
            regex = b"id\x06\x18user\d{20}"
            regex_matches = re.findall(regex, cachecontents)
            keys_found_iter = list(m[4:].decode("utf-8") for m in regex_matches)
            keys_found += keys_found_iter

    if phase == "run":
        for rundump in glob.glob(f"dumps/run-{workload}-{read_policy}"):
            cachecontents = []
            for filename in os.listdir(rundump):
                # Check if the current item is a file
                abs_path = os.path.join(rundump, filename)
                if os.path.isfile(abs_path):
                    with open(abs_path, "rb") as f:
                        cachecontents.append(f.read())

            cachecontents = b"".join(cachecontents)
            regex = b"id\x06\x18user\d{20}"
            regex_matches = re.findall(regex, cachecontents)
            keys_found_iter = list(m[4:].decode("utf-8") for m in regex_matches)
            keys_found += keys_found_iter


    # for key in caches:
    with open(f"dumps/similarity-{workload}-{read_policy}", "w", encoding="utf8") as f:
        similarity = find_similarity_new(keys_found)
        print(f"Similarity for {phase} phase on workload {workload} was {similarity}, with {len(keys_found)} keys in cache")
        f.write(str(similarity))
    
    with open(f"dumps/keys_cached-{workload}-{read_policy}", "w", encoding="utf8") as f:
        num_keys = len(keys_found)
        f.write(num_keys)