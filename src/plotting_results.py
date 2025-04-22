import pandas as pd
import matplotlib.pyplot as plt

# Load the updated results
df = pd.read_csv("/mnt/data/results.csv")

# Plot amortized speedup for each failed node across trials
plt.figure(figsize=(10, 5))
for node in df['FailNode'].unique():
    subset = df[df['FailNode'] == node]
    plt.plot(subset['Trial'], subset['Speedup_pct'], marker='o', label=f'FailNode={node}')
plt.title('Amortized Speedup Across Trials per Failed Node')
plt.xlabel('Trial')
plt.ylabel('Speedup (%)')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# Plot average key distribution across all trials
avg_distribution = df[['A', 'B', 'C']].mean().sort_index()
avg_distribution.plot(kind='bar', figsize=(7, 4))
plt.title('Average Key Distribution Across Nodes')
plt.xlabel('Node')
plt.ylabel('Average Number of Keys')
plt.grid(axis='y')
plt.tight_layout()
plt.show()
