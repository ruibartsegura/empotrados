#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

path = "../src/"

# Read CSV
df = pd.read_csv(path + "cyclictestURJC_lab_s1.csv")
df.columns = df.columns.str.strip()

# Latency column in ns
latencies = df["LATENCIA"]

# Convert to microseconds
latencies_us = latencies / 1000

# Round to integer microseconds
latencies_int = latencies_us.round().astype(int)

# Statistics
std = latencies_int.std()
mean = latencies_int.mean()

# Define bins from minimum up to mean + std
xmin = latencies_int.min()
xmax = int(mean + std)  # convert to integer for bins
bins = np.arange(xmin, xmax + 2)  # +2 to include xmax in the last bin

# Create histogram
plt.figure(figsize=(12, 6))

# Regular frequency
plt.hist(
    latencies_int,
    bins=bins,
    alpha=0.7,
    label=f"Frequency (σ={std:.1f} µs, μ={mean:.1f} µs)",
    edgecolor='black'
)

# Labels and formatting
plt.title("Latency Distribution - Idle Scenario (Real-Time Kernel)")
plt.xlabel("Latency (µs)")
plt.ylabel("Frequency")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.show()
