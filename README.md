# SMOG

## Installation

First install the listed requirements (boost).
Then compile smog by calling `make`.

### Requirements
- lib boost

### Compile

    make smog

## Calling SMOG

You can call `smog --help` to get a list of the available command-line parameters.
The output should look something like this:

    SMOG Usage:
    -h [ --help ]                    Display this help message
    -t [ --threads ] arg (=8)        Number of threads to spawn
    -p [ --pages ] arg (=524288)     Number of pages to allocate
    -d [ --delay ] arg (=1000)       Delay in nanoseconds per thread per 
                                    iteration
    -r [ --rate ] arg                Target dirty rate to automatically adjust 
                                    delay
    -R [ --adjust-timeout ] arg (=0) Timeout in seconds for automatic adjustment

## Affinity on NUMA systems

Setting proper affinity masks for threads and data is crucial to ensure consistent measurements on NUMA systems.
To do this you can use `numactl` like this:

    numactl -C 48,52,56,60,64,68,72,76,80,84,88 --membind=8 ./smog -t 1 -k p -d 0
