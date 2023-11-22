import matplotlib.pyplot as plt

workloads = ['a', 'b', 'c', 'd', 'f']
read_policies = ['random', 'roundrobin', 'hash']

def get_run_miss_rate(workload: str, read_policy: str) -> float:
    with open(f"dumps/mrate-{workload}-{read_policy}", "r", encoding="utf8") as f:
        filedump = f.read()
        
        res = filedump.split("load=")[1]
        finalres = res.split("run=")[1]

        return float(finalres)


def get_similarity(workload: str, read_policy: str) -> float:
    with open(f"dumps/similarity-{workload}-{read_policy}", "r", encoding="utf8") as f:
        filedump = f.read()
        similarity = float(filedump)

        return similarity

def get_keys_cached(workload: str, read_policy: str) -> int:
    with open(f"dumps/keys_cached-{workload}-{read_policy}", "r", encoding="utf8") as f:
        filedump = f.read()
        keys_cached = int(filedump)

        return keys_cached

workload_to_keys_cached = {}

for workload in workloads:
    workload_to_mrate = {}

    for read_policy in read_policies:
        run_missrate = get_run_miss_rate(workload, read_policy)
        similarity = get_similarity(workload, read_policy)
        keys_cached = get_keys_cached(workload, read_policy)

        workload_to_keys_cached[workload] = keys_cached
        workload_to_mrate[read_policy] = (run_missrate, similarity)

    labels, values = zip(*[(value[0], value[1]) for _, value in workload_to_mrate.items()])

    # Plotting
    plt.scatter(labels, values, marker='o')
    plt.ylabel('Similarity')
    plt.xlabel('Miss rate')
    plt.title(f'Workload {workload}')

    # Adding labels for individual points
    for policy in workload_to_mrate:
        label, value = workload_to_mrate[policy]
        plt.text(label, value, f'{policy}', ha='right', va='bottom')

    # Saving the miss rate vs similarity plot
    plt.savefig(f'plot-{workload}.png')
    plt.clf()

categories, values = list(workload_to_keys_cached.keys()), list(workload_to_keys_cached.values())
plt.bar(categories, values)

plt.xlabel('Workload')
plt.ylabel('Num keys cached')
plt.title('Number of keys cached across workloads')

plt.savefig(f'plot-kcached.png')
plt.clf()