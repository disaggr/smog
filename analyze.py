"""Plot the data collected with smog."""
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

PAGE_SIZE = 4096

data_base = Path("data")

results = []

for file_name in data_base.glob("*.csv"):
    df = pd.read_csv(file_name)
    means = df.groupby("thread").mean()

    # print(means["pages"].sum())
    gibs = means["pages"].sum() * PAGE_SIZE / means["elapsed"][0] / 2**30
    print(f"{file_name}: {gibs:.3f}GiBs")
    kernel, delay = file_name.stem.split("_")
    results.append(dict(kernel=kernel, delay=delay, gibs=gibs))

pd.DataFrame(results).plot()
plt.savefig("fig.png")
plt.show()
