"""Run a set of experiments with different kernels and delays."""
from pathlib import Path
import subprocess
from enum import Enum, IntEnum


class KernelType(Enum):
    """Defines the SMOG kernel types."""

    LINEAR = "l"
    RANDOM = "r"
    WRITE = "w"
    POINTERCHASE = "p"
    COLD = "c"
    DIRTY = "d"

class Delay(IntEnum):
    """Defines the configrued SMOG delays."""

    NONE = 0
    LONG = 1000
    SHORT = 100

def smog(kernels: list[KernelType], delay: int, csv_file: Path):
    """Run the smog program with the given configuration."""
    binary = "./smog"
    timeout = 10
    args = [
        binary,
        "-k",
        *[k.value for k in kernels],
        "-t",
        str(len(kernels)),
        "-d",
        str(delay),
        "-c",
        str(csv_file),
    ]
    print(f'+{" ".join(args)}')
    try:
        subprocess.call(args, timeout=timeout)
    except subprocess.TimeoutExpired:
        pass


def main():
    """Run a set of experiments."""
    data_base = Path("data")
    data_base.mkdir(exist_ok=True, parents=True)
    i = 0
    t = len(KernelType) * len(Delay)
    for kernel in KernelType:
        for delay in map(int, Delay):
            i = i + 1
            print(f'[{i}/{t}]', end='')
            kernels = [kernel] * 4
            csv_path = data_base / f"{kernel.name.lower()}_{delay}.csv"
            smog(kernels, delay, csv_path)


if __name__ == "__main__":
    main()
