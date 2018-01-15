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

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_sort.h>

#include "reprompi_bench/sync/time_measurement.h"
#include "reprompi_bench/sync/sync_info.h"
#include "jk_parse_options.h"
#include "reprompi_bench/sync/synchronization.h"

typedef struct {
    long n_rep; /* --repetitions */
    double window_size_sec; /* --window-size */

    int n_fitpoints; /* --fitpoints */
    int n_exchanges; /* --exchanges */

    double wait_time_sec; /* --wait-time */
} reprompi_jk_options_t;

static const int WARMUP_ROUNDS = 5;

static double start_sync = 0; /* current window start timestamp (global time) */
static int* invalid;
static int repetition_counter = 0; /* current repetition index */

// options specified from the command line
static reprompi_jk_options_t parameters;

// final RTT value
static double my_rtt;

//linear model
static double slope, intercept;

static void estimate_rtt(int master_rank, int other_rank, const int n_pingpongs,
        double *rtt) {
    int my_rank, np;
    MPI_Status stat;
    int i;
    double tmp;
    double *rtts = NULL;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    if (my_rank == master_rank) {
        double tstart, tremote;

        /* warm up */
        for (i = 0; i < WARMUP_ROUNDS; i++) {
            tmp = get_time();
            MPI_Send(&tmp, 1, MPI_DOUBLE, other_rank, 0, MPI_COMM_WORLD);
            MPI_Recv(&tmp, 1, MPI_DOUBLE, other_rank, 0, MPI_COMM_WORLD, &stat);
        }

        rtts = (double*) malloc(n_pingpongs * sizeof(double));

        for (i = 0; i < n_pingpongs; i++) {
            tstart = get_time();
            MPI_Send(&tstart, 1, MPI_DOUBLE, other_rank, 0, MPI_COMM_WORLD);
            MPI_Recv(&tremote, 1, MPI_DOUBLE, other_rank, 0, MPI_COMM_WORLD,
                    &stat);
            rtts[i] = get_time() - tstart;
        }

    } else if (my_rank == other_rank) {
        double tlocal = 0, troot;

        /* warm up */
        for (i = 0; i < WARMUP_ROUNDS; i++) {
            MPI_Recv(&tmp, 1, MPI_DOUBLE, master_rank, 0, MPI_COMM_WORLD,
                    &stat);
            tmp = get_time();
            MPI_Send(&tmp, 1, MPI_DOUBLE, master_rank, 0, MPI_COMM_WORLD);
        }

        for (i = 0; i < n_pingpongs; i++) {
            MPI_Recv(&troot, 1, MPI_DOUBLE, master_rank, 0, MPI_COMM_WORLD,
                    &stat);
            tlocal = get_time();
            MPI_Send(&tlocal, 1, MPI_DOUBLE, master_rank, 0, MPI_COMM_WORLD);
        }
    }

    if (my_rank == master_rank) {
        double mean;
        double upperq;
        double cutoff_val;
        double *rtts2;
        int n_datapoints;

        gsl_sort(rtts, 1, n_pingpongs);

        upperq = gsl_stats_quantile_from_sorted_data(rtts, 1, n_pingpongs,
                0.75);
        cutoff_val = 1.5 * upperq;

        //    printf("cutoff=%.20f\n", cutoff_val);
        rtts2 = (double*) calloc(n_pingpongs, sizeof(double));
        n_datapoints = 0;
        for (i = 0; i < n_pingpongs; i++) {
            if (rtts[i] <= cutoff_val) {
                rtts2[i] = rtts[i];
                n_datapoints = i + 1;
            } else {
                break;
            }
        }
        //    printf("n_datapoints=%d\n", n_datapoints);

        //mean = gsl_stats_mean(rtts, 1, n_pingpongs);
        mean = gsl_stats_mean(rtts2, 1, n_datapoints);
        //    printf("mean with outlier detection: %.20f\n", gsl_stats_mean(rtts2, 1, n_datapoints));
        *rtt = mean;

        free(rtts);
        free(rtts2);
    }
}

static void warmup(int root_rank) {
    int my_rank, np;
    MPI_Status status;
    int i, p;
    double tmp;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    if (root_rank == my_rank) {
        for (i = 0; i < WARMUP_ROUNDS; i++) {
            for (p = 0; p < np; p++) {
                if (p != root_rank) {
                    MPI_Send(&tmp, 1, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
                    MPI_Recv(&tmp, 1, MPI_DOUBLE, p, 0, MPI_COMM_WORLD,
                            &status);
                }
            }
        }
    } else {
        for (i = 0; i < WARMUP_ROUNDS; i++) {
            MPI_Recv(&tmp, 1, MPI_DOUBLE, root_rank, 0, MPI_COMM_WORLD,
                    &status);
            MPI_Send(&tmp, 1, MPI_DOUBLE, root_rank, 0, MPI_COMM_WORLD);
        }
    }

}

static void learn_clock(const int root_rank, double *intercept, double *slope,
        const long n_fitpoints, const long n_exchanges, const double my_rtt) {
    int i, j, p;
    int my_rank, np;
    MPI_Status status;

    //  struct timespec ts;
    //
    //  ts.tv_sec  = 0;
    //  ts.tv_nsec = wait_nsec;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    //  printf("rtt of %d : %.20f\n", my_rank, my_rtt);

    *intercept = 0;
    *slope = 0;

    if (my_rank == root_rank) {

        double tlocal, tremote;

        for (j = 0; j < n_fitpoints; j++) {

            for (p = 0; p < np; p++) {
                if (p != root_rank) {
                    for (i = 0; i < n_exchanges; i++) {
                        MPI_Recv(&tremote, 1, MPI_DOUBLE, p, 0, MPI_COMM_WORLD,
                                &status);
                        tlocal = get_time();
                        MPI_Ssend(&tlocal, 1, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
                    }
                }
            }

        }

    } else {

        double *time_var, *local_time, *time_var2;
        double *xfit, *yfit;
        double cov00, cov01, cov11, sumsq;
        int fit;
        double dummy;
        double median, master_time;

        time_var = (double*) calloc(n_exchanges, sizeof(double));
        time_var2 = (double*) calloc(n_exchanges, sizeof(double));
        local_time = (double*) calloc(n_exchanges, sizeof(double));

        xfit = (double*) calloc(n_fitpoints, sizeof(double));
        yfit = (double*) calloc(n_fitpoints, sizeof(double));

        for (j = 0; j < n_fitpoints; j++) {

            for (i = 0; i < n_exchanges; i++) {
                dummy = get_time();
                MPI_Ssend(&dummy, 1, MPI_DOUBLE, root_rank, 0, MPI_COMM_WORLD);
                MPI_Recv(&master_time, 1, MPI_DOUBLE, root_rank, 0,
                        MPI_COMM_WORLD, &status);
                local_time[i] = get_time();
                time_var[i] = local_time[i] - master_time - my_rtt / 2.0;
                time_var2[i] = time_var[i];
            }

            gsl_sort(time_var2, 1, n_exchanges);

            if (n_exchanges % 2 == 0) {
                median = gsl_stats_median_from_sorted_data(time_var2, 1,
                        n_exchanges - 1);
            } else {
                median = gsl_stats_median_from_sorted_data(time_var2, 1,
                        n_exchanges);
            }

            for (i = 0; i < n_exchanges; i++) {
                if (time_var[i] == median) {
                    xfit[j] = local_time[i];
                    yfit[j] = time_var[i];
                    break;
                }
            }

        }

        fit = gsl_fit_linear(xfit, 1, yfit, 1, n_fitpoints, intercept, slope,
                &cov00, &cov01, &cov11, &sumsq);

        //    printf("p%d: model: intercept=%10.5f slope=%.20f\n", my_rank, *intercept,
        //        *slope);

        free(time_var);
        free(time_var2);
        free(local_time);
        free(xfit);
        free(yfit);

    }
}

static inline double jk_get_normalized_time(double local_time) {
    return local_time - (local_time * slope + intercept);
}


static void jk_sync_clocks(void) {
    int p;
    int master_rank;
    double *rtts_s;
    int n_pingpongs = 1000;
    int my_rank, np;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    master_rank = 0;

    rtts_s = (double*) malloc(n_pingpongs * sizeof(double));
    rtts_s[master_rank] = 0.0;

    warmup(master_rank);

    for (p = 0; p < np; p++) {
        if (p != master_rank) {
            estimate_rtt(master_rank, p, n_pingpongs, &rtts_s[p]);
        }
    }

    MPI_Scatter(rtts_s, 1, MPI_DOUBLE, &my_rtt, 1, MPI_DOUBLE, master_rank,
            MPI_COMM_WORLD);
    free(rtts_s);

    learn_clock(master_rank, &intercept, &slope, parameters.n_fitpoints,
            parameters.n_exchanges, my_rtt);

    MPI_Barrier(MPI_COMM_WORLD);
}


static void jk_start_synchronization(void) {
    int is_first = 1;
    double global_time;

    while (1) {
        global_time = jk_get_normalized_time(get_time());

        if (global_time >= start_sync) {
            if (is_first == 1) {
                invalid[repetition_counter] |= FLAG_START_TIME_HAS_PASSED;
            }
            break;
        }
        is_first = 0;
    }
}

static void jk_stop_synchronization(void) {
    double global_time;
    global_time = jk_get_normalized_time(get_time());

    if (global_time > start_sync + parameters.window_size_sec) {
        invalid[repetition_counter] |= FLAG_SYNC_WIN_EXPIRED;
    }

    start_sync += parameters.window_size_sec;
    repetition_counter++;
}

static int* jk_get_local_sync_errorcodes(void) {
    return invalid;
}

static double jk_get_timediff_to_root(double local_time) {
    return local_time - jk_get_normalized_time(local_time);
}


static void jk_print_sync_parameters(FILE* f) {
    fprintf(f, "#@clocksync=JK\n");
    fprintf(f, "#@window_s=%.10f\n", parameters.window_size_sec);
    fprintf(f, "#@fitpoints=%d\n", parameters.n_fitpoints);
    fprintf(f, "#@exchanges=%d\n", parameters.n_exchanges);
    fprintf(f, "#@wait_time_s=%.10f\n", parameters.wait_time_sec);
}


static void jk_init_synchronization(const reprompib_sync_params_t* init_params) {
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

static void jk_finalize_synchronization(void)
{
    free(invalid);
}

static void jk_init_sync_round(void) {  // initialize the first synchronization window
  int my_rank;
  int master_rank = 0;

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if( my_rank == master_rank ) {
      start_sync = get_time() + parameters.wait_time_sec;
  }
  MPI_Bcast(&start_sync, 1, MPI_DOUBLE, master_rank, MPI_COMM_WORLD);

}



static void jk_init_module(int argc, char** argv) {
  reprompib_sync_options_t sync_opts;
  jk_parse_options(argc, argv, &sync_opts);

  parameters.n_exchanges = sync_opts.n_exchanges;
  parameters.n_fitpoints = sync_opts.n_fitpoints;
  parameters.wait_time_sec = sync_opts.wait_time_sec;
  parameters.window_size_sec = sync_opts.window_size_sec;

}


static void jk_cleanup_module(void) {

}


void register_jk_module(reprompib_sync_module_t *sync_mod) {
  sync_mod->name = "JK";
  sync_mod->clocksync = REPROMPI_CLOCKSYNC_JK;
  sync_mod->procsync = REPROMPI_PROCSYNC_WIN;

  sync_mod->init_module = jk_init_module;
  sync_mod->cleanup_module = jk_cleanup_module;

  sync_mod->init_sync = jk_init_synchronization;
  sync_mod->finalize_sync = jk_finalize_synchronization;
  sync_mod->start_sync = jk_start_synchronization;
  sync_mod->stop_sync = jk_stop_synchronization;

  sync_mod->sync_clocks = jk_sync_clocks;
  sync_mod->init_sync_round = jk_init_sync_round;

  sync_mod->get_global_time = jk_get_normalized_time;
  sync_mod->get_errorcodes = jk_get_local_sync_errorcodes;
  sync_mod->print_sync_info = jk_print_sync_parameters;
}


