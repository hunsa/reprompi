/*  ReproMPI Benchmark
 *
 *  Copyright 2015 Alexandra Carpen-Amarie, Sascha Hunold
    Research Group for Parallel Computing
    Faculty of Informatics
    Vienna University of Technology, Austria

<license>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
</license>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include "mpi.h"
#include "mpits.h"
#include "reprompi_bench/misc.h"
//#include "reprompi_bench/sync/clock_sync/synchronization.h"
#include "reprompi_bench/sync/process_sync/process_synchronization.h"
#include "benchmark_job.h"
#include "reprompi_bench/option_parser/option_parser_helpers.h"
#include "reprompi_bench/option_parser/parse_options.h"
#include "reprompi_bench/option_parser/parse_common_options.h"
#include "reprompi_bench/option_parser/parse_timing_options.h"
#include "reprompi_bench/output_management/bench_info_output.h"
#include "reprompi_bench/output_management/runtimes_computation.h"
#include "reprompi_bench/output_management/results_output.h"
#include "collective_ops/collectives.h"
#include "reprompi_bench/utils/keyvalue_store.h"
#include "reprompi_bench/caching/caching.h"
#include "benchmarkCollective.h"

static mpits_clocksync_t cs;

int main(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  // parse arguments and set-up benchmarking jobs
  print_command_line_args(argc, argv);

  //reprompib_register_sync_modules();
  MPITS_Init(MPI_COMM_WORLD, &cs);

  reprompib_register_proc_sync_modules();
  reprompib_register_caching_modules();

  run_collective(argc, argv, &cs);

  //reprompib_deregister_sync_modules();
  MPITS_Finalize();

  reprompib_deregister_proc_sync_modules();
  reprompib_deregister_caching_modules();

  MPI_Finalize();

  return 0;
}
