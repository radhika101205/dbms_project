import csv
import matplotlib.pyplot as plt

# ---- Load CSV data ----
stats = []
with open("pf_stats.csv", newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        row["write_pct"] = int(row["write_pct"])
        row["read_pct"] = int(row["read_pct"])
        row["logicalReads"] = int(row["logicalReads"])
        row["logicalWrites"] = int(row["logicalWrites"])
        row["physicalReads"] = int(row["physicalReads"])
        row["physicalWrites"] = int(row["physicalWrites"])
        stats.append(row)

policies = sorted(set(r["policy"] for r in stats))

def filter_policy(policy):
    return [r for r in stats if r["policy"] == policy]

# ---- Plot physicalReads vs write_pct ----
plt.figure()
for policy in policies:
    rows = filter_policy(policy)
    xs = [r["write_pct"] for r in rows]
    ys = [r["physicalReads"] for r in rows]
    plt.plot(xs, ys, marker="o", label=policy)

plt.xlabel("Write percentage in workload")
plt.ylabel("Physical Reads")
plt.title("PF: Physical Reads vs Write Mix")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("pf_physical_reads.png", dpi=200)

# ---- Plot physicalWrites vs write_pct ----
plt.figure()
for policy in policies:
    rows = filter_policy(policy)
    xs = [r["write_pct"] for r in rows]
    ys = [r["physicalWrites"] for r in rows]
    plt.plot(xs, ys, marker="o", label=policy)

plt.xlabel("Write percentage in workload")
plt.ylabel("Physical Writes")
plt.title("PF: Physical Writes vs Write Mix")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("pf_physical_writes.png", dpi=200)

# ---- Plot logicalReads vs write_pct (for completeness) ----
plt.figure()
for policy in policies:
    rows = filter_policy(policy)
    xs = [r["write_pct"] for r in rows]
    ys = [r["logicalReads"] for r in rows]
    plt.plot(xs, ys, marker="o", label=policy)

plt.xlabel("Write percentage in workload")
plt.ylabel("Logical Reads")
plt.title("PF: Logical Reads vs Write Mix")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("pf_logical_reads.png", dpi=200)

print("Saved plots: pf_physical_reads.png, pf_physical_writes.png, pf_logical_reads.png")
