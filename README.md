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
