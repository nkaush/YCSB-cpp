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

    for cachedump in glob.glob("dumps/rethink*/*.*"):
        _, nodename, ts = cachedump.split("/", 2)
        with open(cachedump, "rb") as f:
            cachecontents = f.read()

        regex = b"id\x06\x18user\d{20}"
        regex_matches = re.findall(regex, cachecontents)
        keys_found = set(m[4:].decode("utf-8") for m in regex_matches)

        dirname = f"cached-keys/{nodename}"
        os.makedirs(dirname, exist_ok=True)

        with open(f"{dirname}/{ts}", "w") as f:
            f.write("\n".join(keys_found))

        caches[nodename] = keys_found

    similarity = find_similarity(caches.values())
    print(similarity)