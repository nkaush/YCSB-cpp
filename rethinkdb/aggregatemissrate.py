import sys
import argparse

from typing import List

def parse_args(args_list: List[str]):
    parser = argparse.ArgumentParser()

    parser.add_argument("-o", "--numops", type=int, default=12000000)
    parser.add_argument("-p", "--phase", type=str, default="load")
    parser.add_argument("-f", "--file", type=str, default="dumps/")

    return parser.parse_args(args_list)


args = parse_args(sys.argv[1:])
total_opps = args.numops
phase = args.phase
total_miss = 0

line = "f"
with open("Missrate.txt", "r") as f:
	while line != "":
		line = f.readline()
		try:
			mrate = int(line.split(":")[1])
		except:
			break
		total_miss+=mrate

with open(args.file, "w") as f:
	print(f"total misses in {phase} phase = {total_miss}. Miss rate = {total_miss/total_opps}")
	f.write(f"total misses in {phase} phase = {total_miss}. Miss rate = {total_miss/total_opps}")
