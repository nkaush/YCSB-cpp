import json
import argparse
import sys

from typing import List

import re

def replace_multiple_spaces(text):
    return re.sub(r'\s+', ' ', text)

def parse_args(args_list: List[str]):
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--phase", type=str, default="load")
    parser.add_argument("-w", "--workload", type=str, default="workloads/workloada")
    parser.add_argument("-rp", "--readpolicy", type=str)
    parser.add_argument("-sf", "--statsfile", type=str)

    return parser.parse_args(args_list)

args = parse_args(sys.argv[1:])
phase = args.phase
workload = args.workload
read_policy = args.readpolicy
stats_file = args.statsfile


with open(stats_file, "r", encoding="utf8") as fp:
    data = fp.read()

    cleaned = ''.join(data.splitlines()[1:])
    cleaned = replace_multiple_spaces(cleaned)
    cleaned = cleaned.replace("\'", "\"")
    cleaned = cleaned.replace("None", "null")
    
    res = json.loads(cleaned)
    with open("dumps/missratesnew.txt", "a") as f:
        tracker = {}
        for replica in res:
            for stat in replica['stats']:
                print(replica['stats'][stat]['serializers']['serializer']['serializer_block_reads']['total'])
                # tracker[replica] = replica['stats'][stat]['serializers']['serializer']['serializer_block_reads']['total']
        print(tracker)
            
        # two=4000000 # TODO read this from the 2 count in the set.
        # f.write(f"{workload}-{read_policy}-{phase}:{one}:{two}:{one/two}")
        

# with open("readmiss.txt") as fp:
#     data = fp.read()
#     one = data.count("1")
#     # two = data.count("2")
#     two=4000000
#     print(f"One: {one}, Two: {two}")
#     with open("dumps/missratesnew.txt", "a") as f:
#         f.write(f"{workload}-{read_policy}-{phase}:{one}:{two}:{one/two}")
        
#         # rm -rf dumps/* && rm -rf node-*/* && rm *.txt