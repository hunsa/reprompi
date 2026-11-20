/*  PGChecker - MPI Performance Guidelines Checker
 *
 *  Copyright 2023 Sascha Hunold, Maximilian Hagn
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

#include <iostream>
#include <fstream>
#include <numeric>
#include <chrono>
#include <filesystem>

#include "mpi.h"
#include "mpits.h"
#include "pgmpi_tune.h"
#include "constants.h"

#include "reprompi_bench/misc.h"
//#include "reprompi_bench/sync/clock_sync/synchronization.h"
//#include "reprompi_bench/sync/clock_sync/utils/communicator_utils.h"
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

#include "pgtunelib_interface.h"
#include "pgcheck_helper.h"
#include "pgcheck_options.h"
#include "pgcheck_input.h"
#include "pgdata.h"
#include "pgdata_printer.h"
#include "pgdata_comparer.h"
#include "comparer/simple_comparer.h"
#include "comparer/violation/violation_comparer.h"
#include "comparer/violation/grouped_violation_comparer.h"
#include "comparer/comparer_factory.h"
#include "utils/argv_manager.h"
#include "utils/statistics_utils.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"
#include "logger/logger.h"

namespace fs = std::filesystem;
const std::string PGCHECKER_ENV_CSV = "PGCHECKER_CSV";
static mpits_clocksync_t cs;

static double get_barrier_runtime(PGCheckOptions &options) {
  double avg_runtime = CONSTANTS::NO_BARRIER_TIME_VALUE;
  int rank;
  // run a barrier test
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  std::string barrier_options =
      "--calls-list=MPI_Barrier --msizes-list=1 --nrep=500 --proc-sync=roundtime "
      "--rt-bench-time-ms=2000 --bcast-nrep=1 --rt-barrier-count=0 --output-file=";
  fs::path outpath = options.get_output_directory();
  outpath /= "barrier.txt";
  barrier_options += outpath;

  Logger::INFO_VERBOSE("Command-Line Barrier:   " + barrier_options);

  std::vector <std::string> barrier_argv_vector;
  auto foo = std::vector<std::string>();
  argv::compose_argv_vector("dummy", barrier_options, foo, &barrier_argv_vector);
  int argc_test = 0;
  char **argv_test;

  argv::convert_vector_to_argv_cstyle(barrier_argv_vector,
                                      &argc_test,
                                      &argv_test);
  pgtune_override_argv_parameter(argc_test, argv_test);
//  for(int i=0; i<argc_test; i++) {
//    std::cout << i << " : " << argv_test[i] << std::endl;
//  }
  run_collective(argc_test, argv_test, &cs);
  argv::free_argv_cstyle(argc_test, argv_test);

  if (rank == CONSTANTS::ROOT_PROCESS_RANK) {
    auto data = new PGData("MPI_Barrier", "default");
    data->read_csv_from_file(outpath.string());
    avg_runtime = StatisticsUtils<double>().median(
        data->get_runtimes_for_count(1));
  }
  MPI_Bcast(&avg_runtime, 1, MPI_DOUBLE, CONSTANTS::ROOT_PROCESS_RANK, MPI_COMM_WORLD);
  return avg_runtime;
}

static void create_intranode_communicator(MPI_Comm old_comm, MPI_Comm *new_comm) {
  int my_rank;
  MPI_Comm_rank(old_comm, &my_rank);
  MPI_Comm_split_type(old_comm, MPI_COMM_TYPE_SHARED, my_rank, MPI_INFO_NULL, new_comm);
}


int main(int argc, char *argv[]) {
  int rank, ppn, nnodes, comm_size;
  MPI_Comm comm_intranode;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  create_intranode_communicator(MPI_COMM_WORLD, &comm_intranode);
  MPI_Comm_size(comm_intranode, &ppn);
  nnodes = comm_size / ppn;

  PGCheckOptions options;
  PGDataPrinter *printer = NULL;
  std::chrono::nanoseconds pg_checker_runtime(0);

  if (rank == CONSTANTS::ROOT_PROCESS_RANK) {
    printer = new PGDataPrinter();
  }

  int parse_status = options.parse(argc, argv, rank == CONSTANTS::ROOT_PROCESS_RANK);
  MPI_Allreduce(MPI_IN_PLACE, &parse_status, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
  if (parse_status == CONSTANTS::FAILURE) {
    Logger::LOG(options.get_usage_string());
    MPI_Comm_free(&comm_intranode);
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  if (rank == CONSTANTS::ROOT_PROCESS_RANK) {
    printer->set_options(options);
  }

  MPITS_Init(MPI_COMM_WORLD, &cs);
  reprompib_register_proc_sync_modules();
  reprompib_register_caching_modules();

  std::ifstream ins(options.get_input_file());
  if (rank == CONSTANTS::ROOT_PROCESS_RANK) {
    if (!ins.good()) {
      Logger::ERROR("input file '" + options.get_input_file() + "' cannot be read");
      Logger::LOG(options.get_usage_string());
      return EXIT_FAILURE;
    }
  }

  double barrier_mean = get_barrier_runtime(options);
  Logger::INFO("Average Barrier Time:   " + std::to_string(barrier_mean * 1000.0) + "ms");

  PGInput input(options.get_input_file());

  std::string csv_conf;

  if (getenv(PGCHECKER_ENV_CSV.c_str()) == nullptr) {
    Logger::WARN("Environment variable '" + PGCHECKER_ENV_CSV + "' is not set, trying \"pgmpi_conf.csv\" in current working directory");
    csv_conf = "pgmpi_conf.csv";
    if (!fs::exists(csv_conf)) {
      Logger::ERROR("cannot find '" + csv_conf + "'");
      return EXIT_FAILURE;
    } 
  } else {
    csv_conf = getenv(PGCHECKER_ENV_CSV.c_str());
    if (!fs::exists(csv_conf)) {
      Logger::ERROR("Environment variable '" + PGCHECKER_ENV_CSV + "' points to a non-existing file");
      return EXIT_FAILURE;
    }
  }

  Logger::INFO("Using configuration file: " + csv_conf);  

  std::ifstream ifs(csv_conf);
  if (!ifs.good()) {
    Logger::ERROR("cannot find '" + csv_conf + "'");
    return EXIT_FAILURE;
  }

  std::string pginfo_data((std::istreambuf_iterator<char>(ifs)),
                          (std::istreambuf_iterator<char>()));

  auto pgtune_interface = PGTuneLibInterface(pginfo_data);

  size_t merge_table_id = 0;

  for (int case_id = 0; case_id < input.get_number_of_test_cases(); case_id++) {
    std::string mpi_coll = input.get_mpi_collective_for_case_id(case_id);

    Logger::INFO_VERBOSE("Case " + std::to_string(case_id) + ": " + mpi_coll);

    auto mod_name = pgtune_interface.get_module_name_for_mpi_collectives(
        mpi_coll);
    std::unordered_map < std::string, PGData * > coll_data;
    for (auto &alg_version : pgtune_interface.get_available_implementations_for_mpi_collectives(mpi_coll)) {
      std::vector <std::string> pgtunelib_argv;
      Logger::INFO("Collecting Data:        " + mod_name + ":" + alg_version);

      std::string call_options = input.get_call_options_for_case_id(case_id);

      std::string tmp_dir = options.get_output_directory();
      std::string tmp_out_file =
          tmp_dir + "/" + "data_" + mpi_coll + "_" + alg_version + ".dat";

      pgtunelib_argv.push_back("--calls-list=" + mpi_coll);
      pgtunelib_argv.push_back("--output-file=" + tmp_out_file);
      pgtunelib_argv.push_back(
          "--module=" + mod_name + "=" + "alg:" + alg_version);
      std::vector <std::string> argv_vector;
      argv::compose_argv_vector(argv[0], call_options, pgtunelib_argv,
                                &argv_vector);

      int argc_test = 0;
      char **argv_test;
      argv::convert_vector_to_argv_cstyle(argv_vector, &argc_test, &argv_test);

      pgtune_override_argv_parameter(argc_test, argv_test);
      Logger::INFO_VERBOSE(get_command_line_args_string(argc_test, argv_test));

      run_collective(argc_test, argv_test, &cs);
      argv::free_argv_cstyle(argc_test, argv_test);

      if (rank == CONSTANTS::ROOT_PROCESS_RANK) {
        auto *data = new PGData(mpi_coll, alg_version);
        data->read_csv_from_file(tmp_out_file);
        coll_data.insert({alg_version, data});
      }
    }

    if (rank == CONSTANTS::ROOT_PROCESS_RANK) {
      auto runtime_start = std::chrono::high_resolution_clock::now();
      Logger::INFO("Collecting Finished:    " + mod_name);
      for (auto comparer_type : options.get_comparer_list()) {
        Logger::INFO("Evaluating Data:        comparer:" + CONSTANTS::COMPARER_NAMES.at(comparer_type));
        std::unique_ptr <PGDataComparer> comparer = ComparerFactory::create_comparer(comparer_type,
                                                                                     options.get_test_type(), mpi_coll,
                                                                                     nnodes, ppn);
        comparer->set_barrier_time(barrier_mean);
        comparer->add_data(coll_data);
        if (printer->print_collective(comparer, comparer_type,
                                      merge_table_id++) != 0) {
          Logger::ERROR("cannot print results");
          return EXIT_FAILURE;
        }
      }
      auto runtime_end = std::chrono::high_resolution_clock::now();
      pg_checker_runtime += std::chrono::duration_cast<std::chrono::nanoseconds>(
          runtime_end - runtime_start);
    }
    merge_table_id = 0;
  }

  if (rank == CONSTANTS::ROOT_PROCESS_RANK) {
    auto runtime_start = std::chrono::high_resolution_clock::now();
    printer->print_summary();
    auto runtime_end = std::chrono::high_resolution_clock::now();
    pg_checker_runtime += std::chrono::duration_cast<std::chrono::nanoseconds>(
        runtime_end - runtime_start);
    Logger::INFO("PGChecker Runtime:      " + duration_to_string(pg_checker_runtime));
    delete printer;
  }

  reprompib_deregister_proc_sync_modules();
  reprompib_deregister_caching_modules();
  MPITS_Finalize();

  MPI_Comm_free(&comm_intranode);

  MPI_Finalize();

  return EXIT_SUCCESS;
}
