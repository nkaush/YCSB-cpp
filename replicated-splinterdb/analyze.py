#!/usr/bin/env python3
from functools import reduce
from typing import List

import glob
import os
import re


def find_similarity(keysets: List[set]) -> float:
    union = reduce(lambda x, y: x | y, keysets)
    size = sum(len(s) for s in keysets)
    count = len(keysets)

    similarity = (count / (count - 1)) * (1 - (len(union) / size))
    return similarity


if __name__ == "__main__":
    caches = {}

    for cachedump in glob.glob("caches/node-*/dump"):
        _, nodename, _ = cachedump.split("/", 2)
        with open(cachedump, "rb") as f:
            cachecontents = f.read()
        
        regex_matches = re.findall(b"user\d*=", cachecontents)
        keys_found = set(m[:-1].decode("utf-8") for m in regex_matches)

        dirname = f"cached-keys/{nodename}"
        os.makedirs(dirname, exist_ok=True)

        with open(f"{dirname}/keys", "w") as f:
            f.write("\n".join(keys_found))

        caches[nodename] = keys_found

    similarity = find_similarity(caches.values())
    print(similarity)
