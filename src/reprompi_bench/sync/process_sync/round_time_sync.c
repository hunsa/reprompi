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
#include <assert.h>
#include <math.h>
#include <getopt.h>
#include <mpi.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

#include "reprompi_bench/misc.h"
#include "reprompi_bench/sync/process_sync/process_synchronization.h"
#include "reprompi_bench/sync/process_sync/reprompi_collectives.h"
//#include "reprompi_bench/sync/clock_sync/synchronization.h"

#include "round_sync_common.h"

//#define ZF_LOG_LEVEL ZF_LOG_INFO
//#define ZF_LOG_LEVEL ZF_LOG_VERBOSE
#define ZF_LOG_LEVEL ZF_LOG_WARN
#include "log/zf_log.h"

typedef struct {
  double time_slot;
} reprompi_roundtime_sync_params_t;


static mpits_clocksync_t* clock_sync_mod; /* pointer to current clock synchronization module */

// options specified from the command line
static reprompi_roundtime_sync_params_t roundtime_parameters;
static reprompi_roundsync_bcast_params_t bcast_parameters;

static const int master_rank = 0;

static int invalid;
static double start_sync    = 0.0;
static double bcast_runtime = 0.0;
static double job_start_time= 0.0;
static int stop_flag        = 0;

static int barrier_sync_mode  = 0;   // boolean: indicates whether to use clock sync (0) or barrier (1)
static int switch_count       = -1;  // when to start switching to barrier, default -1 means never

void roundtimesync_parse_options(int argc, char **argv, reprompi_roundtime_sync_params_t* opts_p) {
    int c;

    static const struct option reprompi_sync_long_options[] = {
            { "rt-bench-time-ms", required_argument, 0, 1000 },
            { "rt-barrier-count", required_argument, 0, 1001 },
            { 0, 0, 0, 0 }
    };
    static const char reprompi_sync_opts_str[] = "";

    optind = 1;
    optopt = 0;
    opterr = 0; // ignore invalid options

    while (1) {

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, reprompi_sync_opts_str, reprompi_sync_long_options,
                &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 1000:
            opts_p->time_slot = atof(optarg) * 1e-3;
            break;
        case 1001:
            switch_count = atoi(optarg);
            break;
        case '?':
             break;
        }
    }

    optind = 1; // reset optind to enable option re-parsing
    opterr = 1; // reset opterr
}



static void roundtimesync_init_sync_round(void) {
  job_start_time     = MPITS_get_time();
  stop_flag          = 0; // all are running
  invalid            = REPROMPI_CORRECT_MEASUREMENT;
}

static void roundtimesync_start_synchronization(MPI_Comm comm) {

  if( barrier_sync_mode == 1 ) {
    PMPI_Barrier(comm);

  } else {
    int is_first = 1;
    double global_time;
    int my_rank;

    invalid = REPROMPI_CORRECT_MEASUREMENT;

    MPI_Comm_rank(comm, &my_rank);

    PMPI_Barrier(comm);
    if (my_rank == master_rank) {
      start_sync = MPITS_get_time() + bcast_runtime * bcast_parameters.bcast_multiplier;
    }
    ReproMPI_Bcast(&start_sync, 1, MPI_DOUBLE, master_rank, comm);

#if ZF_LOG_LEVEL < ZF_LOG_WARN
    global_time = clock_sync_mod->get_global_time(get_time());
    ZF_LOGI("[rank %d] current_time=%20.10f need_to_wait_us=%f", my_rank, global_time, 1e6*(start_sync-global_time));
#endif

    while (1) {
      global_time = clock_sync_mod->get_global_time(MPITS_get_time());

      if (global_time >= start_sync) {
        if (is_first == 1) {
          invalid = REPROMPI_INVALID_MEASUREMENT;
        }
        break;
      }
      is_first = 0;
    }

  }
}

static int roundtimesync_stop_synchronization(MPI_Comm comm) {
  int current_meas_flag = REPROMPI_CORRECT_MEASUREMENT;

  //if( barrier_sync_mode == 0 ) {
    double current_runtime = 0;
    int packet[2];

    current_runtime = MPITS_get_time() - job_start_time;
    if (current_runtime >= roundtime_parameters.time_slot) {
      // stop job
      stop_flag = 1; // someone is raising flag
    }

    packet[0] = invalid;
    packet[1] = stop_flag;

    ReproMPI_Allreduce(MPI_IN_PLACE, packet, 2, MPI_INT, MPI_MAX, comm);

    if (packet[1] == 1) {
      // we ran out of time, check measurement and exit
      if (packet[0] == REPROMPI_CORRECT_MEASUREMENT) {
        current_meas_flag = REPROMPI_OUT_OF_TIME_VALID;
      } else {
        current_meas_flag = REPROMPI_OUT_OF_TIME_INVALID;
      }
    } else {
      if (packet[0] != REPROMPI_CORRECT_MEASUREMENT) {
        current_meas_flag = REPROMPI_INVALID_MEASUREMENT;
      }
    }
  //}


  return current_meas_flag;
}


static void roundtimesync_init_module(int argc, char** argv, mpits_clocksync_t* clock_sync) {
  roundtimesync_parse_options(argc, argv, &roundtime_parameters);
  roundsync_parse_bcast_options(argc, argv, &bcast_parameters);

//  if (clock_sync->clocksync == REPROMPI_CLOCKSYNC_NONE) {
//    reprompib_print_error_and_exit("Cannot use the round-sync process synchronization with the selected clock synchronization method (use \"--clock-sync\" to change it)");
//  }

  clock_sync_mod = clock_sync;
  bcast_runtime = measure_bcast_runtime(MPI_COMM_WORLD, &bcast_parameters);
}


static void roundtimesync_cleanup_module(void) {
}

static int* roundtimesync_get_errorcodes(void) {
  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if (my_rank == 0) {
    fprintf(stderr, "WARNING: Measurement errorcodes are not defined for the round-sync synchronization method.\n");
  }
  return NULL;
}

static void roundtimesync_init_synchronization(const reprompib_sync_params_t* init_params) {

  if( init_params == NULL ) {
    fprintf(stderr, "ERROR: sync init_params are NULL....continuing\n");
  } else {
    if( barrier_sync_mode == 0 && switch_count > -1 && init_params->count >= switch_count ) {
      barrier_sync_mode = 1;
    }
  }

}

static void roundtimesync_finalize_synchronization(void) {
}


static void roundtimesync_sync_print(FILE* f)
{
  fprintf(f, "#@procsync=roundtime\n");
  fprintf(f, "#@bcast_nrep=%ld\n", bcast_parameters.bcast_n_rep);
  fprintf(f, "#@bcast_runtime_s=%.10f\n", bcast_runtime);
  fprintf(f, "#@barrier_switch_count=%d\n", switch_count);
}

void register_roundtimesync_module(reprompib_proc_sync_module_t *sync_mod) {
  sync_mod->name = "roundtime";
  sync_mod->procsync = REPROMPI_PROCSYNC_ROUNDTIMESYNC;

  sync_mod->init_module = roundtimesync_init_module;
  sync_mod->cleanup_module = roundtimesync_cleanup_module;

  sync_mod->init_sync = roundtimesync_init_synchronization;
  sync_mod->finalize_sync = roundtimesync_finalize_synchronization;

  sync_mod->init_sync_round = roundtimesync_init_sync_round;
  sync_mod->start_sync = roundtimesync_start_synchronization;
  sync_mod->stop_sync = roundtimesync_stop_synchronization;

  sync_mod->get_errorcodes = roundtimesync_get_errorcodes;
  sync_mod->print_sync_info = roundtimesync_sync_print;
}



