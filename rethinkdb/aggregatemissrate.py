import sys
import re
import argparse

from typing import List

def parse_args(args_list: List[str]):
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--phase", type=str, default="load")
    parser.add_argument("-f", "--file", type=str, default="dumps/nofilepath.txt")
    parser.add_argument("-w", "--workload", type=str, default="workloads/workloada")
    parser.add_argument("-nr", "--numreplicas", type=int, default=3)
    parser.add_argument("-rp", "--readpolicy", type=str)

    return parser.parse_args(args_list)

def get_num_ops(workload: str, phase: str, num_replicas: int=3) -> int:
	with open(workload, "r", encoding="utf8") as f:
		file_data = f.read()
		opct_pattern = r'operationcount=\d+'
		rfr_pattern=r'readproportion=\d+\.?\d*'

		opct_matches = re.findall(opct_pattern, file_data)
		rfr_matches = re.findall(rfr_pattern, file_data)

		if len(opct_matches) > 0 and len(rfr_matches) > 0:
			opct = float(opct_matches[0][15:])
			rfr = float(rfr_matches[0][15:])

			if phase == "load" or workload[-1] == 'a' or workload[-1] == 'f':
				return opct * num_replicas

			return (opct * rfr * num_replicas) + (opct * (rfr - 1))

args = parse_args(sys.argv[1:])
phase = args.phase
workload = args.workload
read_policy = args.readpolicy
total_opps = get_num_ops(workload, phase)
total_miss = 0

line = "f"
with open(f"dumps/totalmisses-{workload[-1]}-{read_policy}-{phase}", "r") as f:
	while line != "":
		line = f.readline()
		if not line:
            		break
		try:
			chunks=line.split(" ")
			mrate=int(chunks[3])
		except:
			break
		total_miss+=mrate


print(total_opps)


with open(args.file, "a") as f:
	print(f"total misses in {phase} phase = {total_miss}. Total opps in the {phase} phase = {total_opps} Miss rate = {total_miss/total_opps}")
	f.write(f"{phase}={total_miss/total_opps}\n")
