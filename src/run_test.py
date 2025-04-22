import subprocess
import re
import pandas as pd
import matplotlib.pyplot as plt

# Configuration
binary = "./build/FTCache"
nodes = ["NodeA", "NodeB", "NodeC"]
runs_per_node = 5
ttl = 0  # No eviction to isolate recache speedup

# Collect results
records = []
for node in nodes:
    for run in range(1, runs_per_node + 1):
        # Run the NVMe mode (it prints amortized speedup)
        cmd = [binary, "--mode", "nvme", "--fail-node", node, "--ttl", str(ttl)]
        output = subprocess.check_output(cmd, text=True)

        # Parse amortized speedup
        m = re.search(r"Amortized speedup:\s*([\d\.]+)%", output)
        speedup = float(m.group(1)) if m else None

        # Parse key distribution
        dist = dict(re.findall(r"(Node[ABC]):\s*(\d+)", output))
        dist = {k: int(v) for k, v in dist.items()}

        records.append({
            "Failed Node": node,
            "Run": run,
            "Speedup (%)": speedup,
            "NodeA files": dist.get("NodeA"),
            "NodeB files": dist.get("NodeB"),
            "NodeC files": dist.get("NodeC")
        })

# Create DataFrame
df = pd.DataFrame.from_records(records)

# Display raw data
import ace_tools as tools; tools.display_dataframe_to_user(name="Consistency of Amortized Speedup", dataframe=df)

# Plot boxplot of speedup per node
plt.figure(figsize=(8, 4))
df.boxplot(column="Speedup (%)", by="Failed Node")
plt.title("Amortized Speedup Distribution by Failed Node")
plt.suptitle("")  # remove default title
plt.xlabel("Failed Node")
plt.ylabel("Speedup (%)")
plt.show()

# Plot average load distribution
avg_dist = df[["NodeA files", "NodeB files", "NodeC files"]].mean()
plt.figure(figsize=(6, 4))
plt.bar(avg_dist.index, avg_dist.values)
plt.title("Average Key Distribution Across Nodes")
plt.xlabel("Node")
plt.ylabel("Average Files per Node")
plt.show()
