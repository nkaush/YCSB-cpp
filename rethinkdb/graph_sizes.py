import matplotlib.pyplot as plt

WORKLOAD="workloads/workloada"
POLICY="roundrobin"
SIZES=[50, 100, 150, 200, 250]

keycount=[]
for size in SIZES:
    with open(f"dumps/keys_cached-{WORKLOAD[-1]}-{POLICY}-{size}", "r", encoding="utf8") as f:
        keyct = int(f.read())
        keycount.append(keyct)

categories, values = SIZES, keycount

plt.bar(categories, values, width=5.0)

plt.xlabel('Overall Cache size in MB')
plt.ylabel('Num keys cached')
plt.title('Number of keys cached across cache sizes')

# Add y-values as text annotations above each bar
for idx, size in enumerate(SIZES):
    plt.text(size, values[idx], str(values[idx]), ha='center', va='bottom')

plt.savefig(f'plot-kcached.png')
plt.clf()
