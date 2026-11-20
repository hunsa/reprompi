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
   Synchronizing MPI Processes in Space and Time. EuroMPI 2023: 7:1-7:11. https://doi.org/https://doi.org/10.1145/3615318.3615325
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

### `MPI_Barrier`
The default synchronization method. Uses `MPI_Barrier` to synchronize
processes before each measurement.

### `Double_MPI_Barrier`
Calls `MPI_Barrier` twice before each measurement for more robust
synchronization.

### `dissem_barrier`
A dissemination barrier implementation that replaces `MPI_Barrier` to
synchronize processes independent of the underlying MPI library.
The dissemination barrier is always compiled in.

### `window`
Window-based process synchronization. Uses global clocks (obtained via
the clock synchronization library) to align processes within a time
window before each measurement. The window size is set with
`--window-size=<microseconds>`.

### `roundtime`
Round-time synchronization. Uses global clocks to synchronize processes
based on measured round-trip times. Requires a clock synchronization
method (configured via [mpi-time-sync](https://github.com/hunsa/mpi-time-sync/) options). The runtime type can be set to `global` or `local`.

Before each measurement, the root broadcasts a target start time derived from the
measured Bcast run-time, and all processes busy-wait until that global time is reached.

Parameters:
  - `--rt-bench-time-ms=<ms>` time limit in milliseconds for measurements of a given
    message size; once the elapsed time exceeds this limit, measurements for that
    message size are stopped
  - `--rt-barrier-count=<bytes>` message size threshold in bytes above which the
    synchronization switches from clock-based to `MPI_Barrier` (default: never switch)
  - `--bcast-nrep=<n>` number of Bcast repetitions used to estimate the Bcast run-time
    (default: 10)
  - `--bcast-mult=<factor>` multiplier applied to the estimated Bcast run-time to compute
    the target start time (must be >= 1.0, default: 3)
  - `--bcast-meas=<mean|median|max>` statistic used to summarize the measured Bcast
    run-times across processes (default: mean)

### `None`
No process synchronization. Processes start the measurement
independently without any barrier or clock-based alignment.

### Timing procedure

The MPI operation run-time is computed differently depending on the
synchronization method:

- With global clocks (`window`, `roundtime`): run-time is computed as
  the difference between the largest exit time and the first start time
  across all processes (`--runtime-type=global`).
- With barrier-based synchronization: run-time is the largest local
  run-time across all processes (`--runtime-type=local`).

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
