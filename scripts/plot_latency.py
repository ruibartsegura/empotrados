#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
from glob import glob

path = "../src/"
csv_files = glob(os.path.join(path, "*.csv"))  # Todos los CSV en el directorio

plt.figure(figsize=(12, 6))

colors = ['blue', 'green', 'red', 'orange', 'purple', 'brown']  # colores para diferenciar archivos
for i, file in enumerate(csv_files):
    df = pd.read_csv(file)
    df.columns = df.columns.str.strip()

    # Latencias en ns -> microsegundos
    latencies_us = df["LATENCIA"] / 1000
    latencies_int = latencies_us.round().astype(int)

    # Estadísticas
    std = latencies_int.std()
    mean = latencies_int.mean()

    # Definir bins comunes (mínimo y máximo entre todos los archivos)
    xmin = latencies_int.min()
    xmax = int(mean + 3 * std)
    bins = np.arange(xmin, xmax + std, 2)

    plt.xticks(np.arange(xmin, xmax + std, 2))  # ticks de 2 en 2
    # Histograma
    plt.hist(
        latencies_int,
        bins=bins,
        alpha=0.5,
        color=colors[i % len(colors)],
        label=f"{os.path.basename(file)} (σ={std:.1f} µs, μ={mean:.1f} µs)"
    )

# Labels y formato
plt.title("Latency Distribution - All Scenarios")
plt.xlabel("Latency (µs)")
plt.ylabel("Frequency")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.show()
