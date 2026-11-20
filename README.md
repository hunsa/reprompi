# ReproMPI Benchmark

## Introduction

The ReproMPI Benchmark is a tool designed to accurately measure the
run-time of **MPI blocking collective** operations such as `MPI_Bcast`,
`MPI_Allgather`, `MPI_Reduce`, etc. 

# References 

1. Sascha Hunold, Alexandra Carpen-Amarie:
   On the Impact of Synchronizing Clocks and Processes on Benchmarking MPI Collectives. EuroMPI 2015: 8:1-8:10. https://doi.org/10.1145/2802658.2802662
2. Sascha Hunold, Alexandra Carpen-Amarie, Jesper Larsson Träff:
   Reproducible MPI Micro-Benchmarking Isn't As Easy As You Think. EuroMPI/ASIA 2014: 69. https://doi.org/10.1145/2642769.2642785
3. Sascha Hunold, Alexandra Carpen-Amarie:
   Reproducible MPI Benchmarking is Still Not as Easy as You Think. IEEE Trans. Parallel Distributed Syst. 27(12): 3617-3630 (2016). https://doi.org/10.1109/TPDS.2016.2539167
4. Sascha Hunold, Alexandra Carpen-Amarie:
   Hierarchical Clock Synchronization in MPI. CLUSTER 2018: 325-336. https://doi.org/10.1109/CLUSTER.2018.00050
5. Sascha Hunold, Alexandra Carpen-Amarie:
   Autotuning MPI Collectives using Performance Guidelines. HPC Asia 2018: 64-74. https://doi.org/10.1145/3149457.3149461
6. Joseph Schuchart, Sascha Hunold, George Bosilca:
   Synchronizing MPI Processes in Space and Time. EuroMPI 2023: 7:1-7:11. https://doi.org/10.1145/3615318.3615325
## Components

- `mpibenchmark`: actual MPI benchmark for collectives
- [`pgchecker`](https://github.com/hunsa/reprompi/tree/main/src/pgcheck/): performance guideline checker

## Installation

For installing `pgchecker`, check the [README](https://github.com/hunsa/reprompi/tree/main/src/pgcheck/README.md)

Prerequisites for installing the ReproMPI benchmark:
- an MPI library 
- CMake (version >= 3.22)  
- GSL libraries 

## Basic installation

```
  git clone https://github.com/hunsa/reprompi.git
  cd reprompi
  cmake -B build
  cmake --build build
```

The `mpibenchmark` executable is then available in `build/bin`.

For specific configuration options check the *Benchmark Configuration* section.

## Running the ReproMPI Benchmark

The ReproMPI code is designed to serve two specific purposes:

## Benchmarking of MPI collective calls
The most common usage scenario of the benchmark is to specify an MPI
collective function to be benchmarked, a (list of) message sizes and
the *number of measurement repetitions* for each test, as in the
following example.

```
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast,MPI_Allgather \
             --msizes-list=8,1024,2048  --nrep=10
```

## Command-line Options

### Common Options

  - `-h` print help
  - `-v` print run-times measured for each process
  - `--msizes-list=<values>` list of comma-separated message sizes in
    Bytes, e.g., `--msizes-list=10,1024`
  - `--msize-interval=min=<min>,max=<max>,step=<step>` list of power
    of 2 message sizes as an interval between $2^{min}$ and $2^{max}$,
    with $2^{step}$ distance between values, e.g.,
    `--msize-interval=min=1,max=4,step=1`
  - `--calls-list=<args>` list of comma-separated MPI calls to be
    benchmarked, e.g., `--calls-list=MPI_Bcast,MPI_Allgather`
  - `--root-proc=<process_id>` root node for collective operations
  - `--operation=<mpi_op>` MPI operation applied by collective
    operations (where applicable), e.g., `--operation=MPI_BOR`.

    Supported operations: `MPI_BOR`, `MPI_BAND`, `MPI_LOR`, `MPI_LAND`,
    `MPI_MIN`, `MPI_MAX`, `MPI_SUM`, `MPI_PROD`
  - `--datatype=<mpi_type>` MPI datatype used by collective
    operations, e.g., `--datatype=MPI_CHAR`.

    Supported datatypes: `MPI_BYTE`, `MPI_CHAR`, `MPI_INT`, `MPI_FLOAT`, `MPI_DOUBLE`
  - `--shuffle-jobs` shuffle experiments before running the benchmark
  - `-f | --input-file=<path>` input file containing the list of
    benchmarking jobs (tuples of MPI function, message size, number of
    repetitions). It replaces all the other common options.
  - `--output-file=<path>` path to the output file where results are written
  - `--pingpong-ranks=<rank1>,<rank2>` two comma-separated process ranks
    used for ping-pong operations
  
  
### Options Related to the Window-based Synchronization

  - `--window-size=<win>` window size in microseconds for Window-based synchronization
  - `--wait-time=<us>` time in microseconds to wait before the first window starts


### Specific Options for the ReproMPI Benchmark

  - `--nrep=<nrep>` set number of experiment repetitions
  - `--summary=<args>` list of comma-separated data summarizing
    methods (mean, median, min, max, var, stddev), e.g., `--summary=mean,max`


## Supported Collective Operations:
### MPI Collectives

  - `MPI_Allgather`
  - `MPI_Allreduce`
  - `MPI_Alltoall`
  - `MPI_Barrier`
  - `MPI_Bcast`
  - `MPI_Exscan`
  - `MPI_Gather`
  - `MPI_Reduce`
  - `MPI_Reduce_scatter`
  - `MPI_Reduce_scatter_block`
  - `MPI_Scan`
  - `MPI_Scatter`


## Process Synchronization Methods

The synchronization method is selected via `--proc-sync=<method>`.
Available methods: `MPI_Barrier` (default), `Double_MPI_Barrier`, `dissem_barrier`,
`window`, `roundtime`, `None`.

### `MPI_Barrier`
The default synchronization method. Uses `MPI_Barrier` to synchronize
processes before each measurement.

```
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=MPI_Barrier
```

### `Double_MPI_Barrier`
Calls `MPI_Barrier` twice before each measurement for more robust
synchronization.

```
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=Double_MPI_Barrier
```

### `dissem_barrier`
A dissemination barrier implementation that replaces `MPI_Barrier` to
synchronize processes independent of the underlying MPI library.
The dissemination barrier is always compiled in.

```
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=dissem_barrier
```

### `window`
Window-based process synchronization. Uses global clocks (obtained via
the clock synchronization library) to align processes within a time
window before each measurement.

Parameters:
  - `--window-size=<us>` size of each measurement window in microseconds (default: 1000)
  - `--wait-time=<us>` time in microseconds to wait before the first window starts (default: 1000)

The clock synchronization algorithm is selected with the `MPITS_PARAMS` environment variable
(see [mpi-time-sync](https://github.com/hunsa/mpi-time-sync/)); if it is not set, `HCA3O` is used.

```
export MPITS_PARAMS="--clock-sync=HCA3O --params=options:skampi_offset@5@20"
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=window --window-size=100 --wait-time=1000 --runtime-type=global
```

With `window`, the output contains an additional `errorcode` column for each repetition:
`0` means the measurement is valid, `1` means a process reached the window after it had
already started, `2` means the measurement did not finish within its window (`3` = both).
Repetitions with a non-zero error code should be discarded; if many repetitions are invalid,
increase `--window-size`.

### `roundtime`
Round-time synchronization. Uses global clocks to synchronize processes
based on measured round-trip times. Requires a clock synchronization
method (configured via [mpi-time-sync](https://github.com/hunsa/mpi-time-sync/) options,
see `window` above). The runtime type can be set to `global` or `local`.

`roundtime` is a mix of time-based (clock-based) synchronization and barrier synchronization:

- **Small messages: time-based.** Before each measurement, the root broadcasts a target start
  time derived from the measured Bcast run-time, and all processes busy-wait until that global
  time is reached. This gives a precise common start, which matters most when the operation
  itself is short.
- **Large messages: `MPI_Barrier`.** For message sizes at or above the threshold set with
  `--rt-barrier-count`, processes are synchronized with `MPI_Barrier` instead. For long-running
  operations, the overhead of the time-based scheme does not pay off, and the small skew left by
  a barrier is negligible compared to the run-time.

Once `roundtime` has switched to `MPI_Barrier`, it stays in barrier mode for the rest of the run.
Message sizes are processed in the given order, so list them in increasing order (and do not use
`--shuffle-jobs`) if smaller sizes should still use time-based synchronization.
If `--rt-barrier-count` is not given, time-based synchronization is used for all message sizes.

The time limit `--rt-bench-time-ms` applies in both modes. Hence, `--rt-barrier-count=0` uses
`MPI_Barrier` for all message sizes, but still stops measuring a message size once its time budget
is used up, even if `--nrep` repetitions have not been reached. This is barrier synchronization
with a time limit per message size, which `--proc-sync=MPI_Barrier` does not provide.
This is the recommended setting for `pgchecker`.

Parameters:
  - `--rt-bench-time-ms=<ms>` time limit in milliseconds for measurements of a given
    message size; once the elapsed time exceeds this limit, measurements for that
    message size are stopped
  - `--rt-barrier-count=<count>` message size threshold (as count of elements of the MPI
    datatype, i.e., bytes when using `MPI_BYTE`/`MPI_CHAR`) at or above which the
    synchronization switches from time-based to `MPI_Barrier` (default: never switch)
  - `--bcast-nrep=<n>` number of Bcast repetitions used to estimate the Bcast run-time
    (default: 10)
  - `--bcast-mult=<factor>` multiplier applied to the estimated Bcast run-time to compute
    the target start time (must be >= 1.0, default: 3)
  - `--bcast-meas=<mean|median|max>` statistic used to summarize the measured Bcast
    run-times across processes (default: mean)

Examples:
```
# basic round-time synchronization, spend at most 1 s per message size
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=roundtime --rt-bench-time-ms=1000 --runtime-type=global

# tune the start-time estimation: 10 Bcasts, use the median, start 3x the Bcast time in the future
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=roundtime --rt-bench-time-ms=1000 \
       --bcast-nrep=10 --bcast-meas=median --bcast-mult=3

# time-based sync for 8, barrier for 1024 (switch at count >= 1000)
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=roundtime --rt-bench-time-ms=1000 --rt-barrier-count=1000

# MPI_Barrier for all message sizes, at most 1 s per message size
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=roundtime --rt-bench-time-ms=1000 --rt-barrier-count=0
```

### `None`
No process synchronization. Processes start the measurement
independently without any barrier or clock-based alignment.

```
mpirun -np 4 ./build/bin/mpibenchmark --calls-list=MPI_Bcast --msizes-list=8,1024 --nrep=100 \
       --proc-sync=None
```

### Timing procedure

The MPI operation run-time is computed differently depending on the
synchronization method:

- `--runtime-type=local` (default, for all methods): run-time is the largest local
  run-time across all processes.
- `--runtime-type=global`: run-time is computed as the difference between the
  largest exit time and the first start time across all processes, using global
  clocks. This is the recommended setting for `window` and `roundtime`, and it
  must be given explicitly.

## List of Compilation Flags

This is the full list of compilation flags that can be used to control
all the previously detailed configuration parameters.

```
 ENABLE_BENCH_TESTS               OFF
 ENABLE_DEBUGGING                 OFF
 ENABLE_DOUBLE_BARRIER            OFF
 ENABLE_GETTIME_MONOTONIC         OFF
 ENABLE_GETTIME_REALTIME          OFF
 ENABLE_LOGGING                   OFF
 ENABLE_PGCHECKER                 OFF
 ENABLE_SANITY_CHECK_CLOCK        OFF
 SET_BUFFER_ALIGNMENT
 SET_PRINT_MSIZES_BYTES           OFF
```

- `ENABLE_SANITY_CHECK_CLOCK`: enable sanity checks for the clock synchronization

- `ENABLE_BENCH_TESTS`: enable the compilation of benchmark tests (e.g., for testing the dictionary implementation)

- `ENABLE_DEBUGGING`: adds the `-g` compiler flag to include debug symbols in the binary

- `ENABLE_LOGGING`: enable logging output

- `ENABLE_PGCHECKER`: enable the performance guidelines checker component

- `SET_BUFFER_ALIGNMENT`: set the buffer alignment in bytes for collective operations. Typically, use
the cache line size of the CPU, e.g., 64 bytes for x86-64 architectures.

- `SET_PRINT_MSIZES_BYTES`: print message sizes in bytes in the benchmark output. By default, ReproMPI print the count of MPI datatypes used by collective operations.

## Clock Synchronization Algorithms

The ReproMPI benchmark uses the [mpi-time-sync](https://github.com/hunsa/mpi-time-sync/) library to synchronize processes and to obtain global clocks. Clock synchronization options (e.g., algorithm selection, number of ping-pongs) are documented in the mpi-time-sync repository.
