import sys
import argparse

from typing import List

def parse_args(args_list: List[str]):
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--phase", type=str, default="load")
    parser.add_argument("-w", "--workload", type=str, default="workloads/workloada")
    parser.add_argument("-rp", "--readpolicy", type=str)

    return parser.parse_args(args_list)

args = parse_args(sys.argv[1:])
phase = args.phase
workload = args.workload
read_policy = args.readpolicy

with open("readmiss.txt") as fp:
    data = fp.read()
    one = data.count("1")
    # two = data.count("2")
    two=4000000
    print(f"One: {one}, Two: {two}")
    with open("dumps/missratesnew.txt", "a") as f:
        f.write(f"{workload}-{read_policy}-{phase}:{one}:{two}:{one/two}")