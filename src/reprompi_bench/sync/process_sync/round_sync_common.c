
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <mpi.h>

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

#include "mpits.h"
#include "reprompi_bench/misc.h"
#include "round_sync_common.h"
#include "reprompi_collectives.h"

//#define ZF_LOG_LEVEL ZF_LOG_INFO
//#define ZF_LOG_LEVEL ZF_LOG_VERBOSE
#define ZF_LOG_LEVEL ZF_LOG_WARN
#include "log/zf_log.h"


double measure_bcast_runtime(MPI_Comm comm, reprompi_roundsync_bcast_params_t *parameters) {
  int my_rank;
  double bcast_runtime;
  int i = 0;
  double* bcast_times = NULL;
  double bcast_mean_rt, bcast_median_rt;
  int master_rank = 0;

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  bcast_runtime = MPITS_get_time(); // that's just used as a dummy here
  bcast_times = (double*) calloc(parameters->bcast_n_rep, sizeof(double));

  for (i = 0; i < parameters->bcast_n_rep; i++) {
    MPI_Barrier(MPI_COMM_WORLD);
    bcast_times[i] = MPITS_get_time();
    ReproMPI_Bcast(&bcast_runtime, 1, MPI_DOUBLE, master_rank, MPI_COMM_WORLD);
    bcast_times[i] = MPITS_get_time() - bcast_times[i];
  }

  gsl_sort(bcast_times, 1, parameters->bcast_n_rep);
  bcast_mean_rt = gsl_stats_mean(bcast_times, 1, parameters->bcast_n_rep);
  bcast_median_rt = gsl_stats_quantile_from_sorted_data(bcast_times, 1, parameters->bcast_n_rep, 0.5);
  ZF_LOGV("[rank %d] Bcast times [us] mean=%f median=%f min=%f max=%f", my_rank, 1e6 * bcast_mean_rt,
      1e6 * bcast_median_rt, 1e6 * bcast_times[0], 1e6 * bcast_times[parameters->bcast_n_rep - 1]);

  switch (parameters->bcast_meas) {
  case BCAST_MEASURE_MAX:
    MPI_Reduce(&(bcast_times[parameters->bcast_n_rep - 1]), &bcast_runtime, 1, MPI_DOUBLE, MPI_MAX, master_rank,
        MPI_COMM_WORLD);
    break;
  case BCAST_MEASURE_MEDIAN:
    MPI_Reduce(&bcast_median_rt, &bcast_runtime, 1, MPI_DOUBLE, MPI_MAX, master_rank, MPI_COMM_WORLD);
    break;
  case BCAST_MEASURE_MEAN:
  default:
    MPI_Reduce(&bcast_mean_rt, &bcast_runtime, 1, MPI_DOUBLE, MPI_MAX, master_rank, MPI_COMM_WORLD);
    break;
  }

  if (my_rank == 0) {
    ZF_LOGI("[rank %d] Bcast run-time estimate [us]: %f", my_rank, bcast_runtime);
  }

  free(bcast_times);

  return bcast_runtime;
}

void roundsync_parse_bcast_options(int argc, char **argv, reprompi_roundsync_bcast_params_t* opts_p) {
    int c;

    static const struct option reprompi_sync_long_options[] = {
            { "bcast-mult", required_argument, 0, REPROMPI_ARGS_PROCSYNC_BCAST_MULTIPLIER },
            { "bcast-nrep", required_argument, 0, REPROMPI_ARGS_PROCSYNC_BCAST_NREP },
            { "bcast-meas", required_argument, 0, REPROMPI_ARGS_PROCSYNC_BCAST_MEASURE },
            { 0, 0, 0, 0 }
    };
    static const char reprompi_sync_opts_str[] = "";

    opts_p->bcast_multiplier = BCAST_MUTIPLIER;
    opts_p->bcast_n_rep      = BCAST_NREP;
    opts_p->bcast_meas       = BCAST_MEASURE_MEAN;

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
        case REPROMPI_ARGS_PROCSYNC_BCAST_MULTIPLIER:
            opts_p->bcast_multiplier = atof(optarg);
            break;
        case REPROMPI_ARGS_PROCSYNC_BCAST_NREP:
            opts_p->bcast_n_rep = atol(optarg);
            break;
        case REPROMPI_ARGS_PROCSYNC_BCAST_MEASURE:
              if (strcmp(optarg, "mean") == 0) {
                opts_p->bcast_meas = BCAST_MEASURE_MEAN;
              } else if (strcmp(optarg, "median") == 0) {
                opts_p->bcast_meas = BCAST_MEASURE_MEDIAN;
              } else if (strcmp(optarg, "max") == 0) {
                opts_p->bcast_meas = BCAST_MEASURE_MAX;
              } else {
                char msg[200];
                sprintf(msg, "unknown bcast measure type: %s", optarg);
                reprompib_print_warning(msg);
              }
              break;
        case '?':
             break;
        }
    }

    // check for errors
    if (opts_p->bcast_multiplier < 1.0) {
      reprompib_print_error_and_exit("bcast-mult error: bcast multiplier must be >= 1.0");
    }
    if (opts_p->bcast_n_rep <= 0) {
      reprompib_print_error_and_exit("bcast-nrep error: we need to do at least 1 bcast");
    }

    optind = 1; // reset optind to enable option re-parsing
    opterr = 1; // reset opterr
}



