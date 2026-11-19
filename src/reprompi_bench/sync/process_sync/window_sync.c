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
#include "mpits.h"
#include "reprompi_bench/misc.h"
#include "reprompi_bench/sync/process_sync/process_synchronization.h"
//#include "reprompi_bench/sync/clock_sync/synchronization.h"

typedef struct {
    long n_rep; /* --repetitions */
    double window_size_sec; /* --window-size */
    double wait_time_sec; /* --wait-time */
} reprompi_winsync_params_t;
const double REPROMPI_SYNC_WAIT_TIME_SEC_DEFAULT = 1e-3;
const double REPROMPI_SYNC_WIN_SIZE_SEC_DEFAULT = 1e-3;


enum {
    FLAG_START_TIME_HAS_PASSED = 0x1,
    FLAG_SYNC_WIN_EXPIRED = 0x2
};


static double start_sync = 0;       /* current window start timestamp (global time) */
static int repetition_counter = 0;  /* current repetition index */
static int* invalid;

static mpits_clocksync_t* clock_sync_mod; /* pointer to current clock synchronization module */

// options specified from the command line
static reprompi_winsync_params_t parameters;



void winsync_parse_options(int argc, char **argv, reprompi_winsync_params_t* opts_p) {
    int c;
    enum {
      REPROMPI_ARGS_WINSYNC_WIN_SIZE = 1200,
      REPROMPI_ARGS_WINSYNC_WAIT_TIME
    };

    static const struct option reprompi_sync_long_options[] = {
            { "window-size", required_argument, 0, REPROMPI_ARGS_WINSYNC_WIN_SIZE },
            { "wait-time", required_argument, 0, REPROMPI_ARGS_WINSYNC_WAIT_TIME },
            { 0, 0, 0, 0 }
    };
    static const char reprompi_sync_opts_str[] = "";

    opts_p->window_size_sec = REPROMPI_SYNC_WIN_SIZE_SEC_DEFAULT;
    opts_p->wait_time_sec = REPROMPI_SYNC_WAIT_TIME_SEC_DEFAULT;

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
        case REPROMPI_ARGS_WINSYNC_WIN_SIZE: /* window size */
            opts_p->window_size_sec = atof(optarg) * 1e-6;
            break;
        case REPROMPI_ARGS_WINSYNC_WAIT_TIME: /* wait time before starting the first measurement  (in usec) */
            opts_p->wait_time_sec = atof(optarg) * 1e-6;
            break;

        case '?':
             break;
        }
    }

    // check for errors
    if (opts_p->window_size_sec <= 0) {
      reprompib_print_error_and_exit("Invalid window size (should be positive)");
    }
    if (opts_p->wait_time_sec <= 0) {
      reprompib_print_error_and_exit("Invalid wait time before the first window (should be positive)");
    }

    optind = 1; // reset optind to enable option re-parsing
    opterr = 1; // reset opterr
}


static void window_init_synchronization(const reprompib_sync_params_t* init_params) {
    int i;
    parameters.n_rep = init_params->nrep;

    // initialize array of flags for invalid-measurements;
    invalid  = (int*)calloc(parameters.n_rep, sizeof(int));
    for(i = 0; i < parameters.n_rep; i++)
    {
        invalid[i] = 0;
    }
    repetition_counter = 0;
}


static void window_finalize_synchronization(void)
{
    free(invalid);
}


// initialize first window
static void window_init_sync_round(void) {
  int my_rank;
  int master_rank = 0;

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  if( my_rank == master_rank ) {
    // we need a normalized time to use across processes as a start window
    // (the local time on the master rank needs to be normalized to work as a reference time)
    start_sync = clock_sync_mod->get_global_time(MPITS_get_time()) + parameters.wait_time_sec;
   // printf("start=%20.12f wait=%f win=%f\n", start_sync, parameters.wait_time_sec, parameters.window_size_sec);

  }
  PMPI_Bcast(&start_sync, 1, MPI_DOUBLE, master_rank, MPI_COMM_WORLD);

}

static void window_start_synchronization(MPI_Comm comm)
{
    int is_first = 1;
    double global_time;

    while(1) {
        //global_time = hca_get_normalized_time(hca_get_adjusted_time());
        global_time = clock_sync_mod->get_global_time(MPITS_get_time());
        if( is_first < 10 ) {
          //printf("global=%20.12f \n", global_time);
          //sleep(1);
        }
        if( global_time >= start_sync ) {
            if( is_first == 1 ) {
                invalid[repetition_counter] |= FLAG_START_TIME_HAS_PASSED;
            }
            break;
        }
        is_first = 0;
    }
}


static int window_stop_synchronization(MPI_Comm comm)
{
    double global_time;
   // global_time = hca_get_normalized_time(hca_get_adjusted_time());
    global_time = clock_sync_mod->get_global_time(MPITS_get_time());


    if( global_time > start_sync + parameters.window_size_sec ) {
        invalid[repetition_counter] |= FLAG_SYNC_WIN_EXPIRED;
    }

    start_sync += parameters.window_size_sec;
    repetition_counter++;

    return REPROMPI_CORRECT_MEASUREMENT;
}

static void window_init_module(int argc, char** argv, mpits_clocksync_t* clock_sync) {
  winsync_parse_options(argc, argv, &parameters);

  if (clock_sync->clocksync == MPITS_CLOCKSYNC_NONE) {
    //reprompib_print_error_and_exit("Cannot use window-based process synchronization with the selected clock synchronization method (use \"--clock-sync\" to change it)");
    reprompib_print_warning("beware, window-based process synchronization only works with synchronized clocks, and no clocksync was selected");
  }

  clock_sync_mod = clock_sync;
}


static void window_cleanup_module(void) {
}


static int* get_local_sync_errorcodes(void)
{
    return invalid;
}

static void window_sync_print(FILE* f)
{
  fprintf(f, "#@procsync=window\n");
  fprintf(f, "#@window_s=%.10f\n", parameters.window_size_sec);
  fprintf(f, "#@wait_time_s=%.10f\n", parameters.wait_time_sec);
}

void register_window_module(reprompib_proc_sync_module_t *sync_mod) {
  sync_mod->name = "window";
  sync_mod->procsync = REPROMPI_PROCSYNC_WIN;

  sync_mod->init_module = window_init_module;
  sync_mod->cleanup_module = window_cleanup_module;

  sync_mod->init_sync = window_init_synchronization;
  sync_mod->finalize_sync = window_finalize_synchronization;

  sync_mod->init_sync_round = window_init_sync_round;
  sync_mod->start_sync = window_start_synchronization;
  sync_mod->stop_sync = window_stop_synchronization;

  sync_mod->get_errorcodes = get_local_sync_errorcodes;
  sync_mod->print_sync_info = window_sync_print;
}



