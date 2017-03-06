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
#include <stdlib.h>
#include "mpi.h"

#include "collective_ops/collectives.h"
#include "reprompi_bench/misc.h"
#include "benchmark_job.h"

static const int LEN_JOB_BATCH = 40;
static const int OUTPUT_ROOT_PROC = 0;

void cleanup_job_list(job_list_t jobs) {
    if (jobs.job_indices) {
        free(jobs.job_indices);
    }
    if (jobs.jobs) {
        free(jobs.jobs);
    }
}


/*
 * Read jobs from input file into jlist and return number of jobs
 *
 */
int read_input_jobs(char* file_name, job_list_t* jlist) {
    FILE* file;
    int result = 0;
    int expected_result = 0;
    int len_jobs;
    char mpi_call[100];
    long msize, nrep;
    int mpi_call_index;

    jlist->jobs = (job_t*) malloc(LEN_JOB_BATCH * sizeof(job_t));

    len_jobs = 0;
    file = fopen(file_name, "r");
    if (file) {

        while (1) {
            result = fscanf(file, "%s %ld %ld", mpi_call, &msize, &nrep);

            /* number of job parameters for MPI_Barrier sync: MPI call, msize, nrep */
            expected_result = 3;

            if (result == EOF) {
                break;
            }
            if (result != expected_result) /* incorrectly formatted file */
            {
                fprintf(stderr, "Incorrectly formatted input file: %s\n",
                        file_name);
                break;
            }

            //printf("%s %ld %ld\n", mpi_call, msize, nrep);

            // fill in job info
            mpi_call_index = get_call_index(mpi_call);
            if (mpi_call_index == -1) {
                /* unknown MPI call - read the next line */
                fprintf(stderr, "Unknown MPI call: %s\n", mpi_call);
                continue;
            }
            jlist->jobs[len_jobs].call_index = mpi_call_index;
            jlist->jobs[len_jobs].msize = msize;
            jlist->jobs[len_jobs].n_rep = nrep;

            len_jobs++;

            // increase number of allocated jobs
            if (len_jobs % LEN_JOB_BATCH == 0) {
                jlist->jobs = (job_t*) realloc(jlist->jobs,
                        (len_jobs + LEN_JOB_BATCH) * sizeof(job_t));
            }
        }

        fclose(file);
    }

    if (len_jobs > 0) {
        jlist->jobs = (job_t*) realloc(jlist->jobs, len_jobs * sizeof(job_t));
    }

    jlist->n_jobs = len_jobs;
    return len_jobs;
}

void generate_job_list(reprompib_options_t opts, job_list_t* jlist) {
    int sizeindex, cindex, i;
    int my_rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    jlist->n_jobs = 0;
    jlist->job_indices = NULL;
    jlist->jobs = NULL;

    /* read jobs from input file and ignore other options */
    if (opts.common_opt.input_file != NULL) {
        read_input_jobs(opts.common_opt.input_file, jlist);
    } else {
        /* create job list according to command line options */

        if (opts.common_opt.n_msize > 0 && opts.common_opt.n_calls > 0) /* we have jobs to run */
        {
            // create the benchmark jobs
            jlist->jobs = (job_t*) malloc(
                    opts.common_opt.n_calls * opts.common_opt.n_msize * sizeof(job_t));

            i = 0;
            for (sizeindex = 0; sizeindex < opts.common_opt.n_msize; sizeindex++) {
                for (cindex = 0; cindex < opts.common_opt.n_calls; cindex++) {
                    jlist->jobs[i].msize = opts.common_opt.msize_list[sizeindex];
                    jlist->jobs[i].call_index = opts.common_opt.list_mpi_calls[cindex];
                    jlist->jobs[i].n_rep = opts.n_rep;

                    i++;
                }
            }

            jlist->n_jobs = i;
        }
    }
    if (jlist->n_jobs > 0) {

        // shuffle the list of job indices
        jlist->job_indices = (int*) malloc(jlist->n_jobs * sizeof(int));

        if (my_rank == OUTPUT_ROOT_PROC) {
            for (i = 0; i < jlist->n_jobs; i++) {
                jlist->job_indices[i] = i;
            }
            if (opts.common_opt.enable_job_shuffling > 0) {
                shuffle(jlist->job_indices, jlist->n_jobs);
            }

        }
        MPI_Bcast(jlist->job_indices, jlist->n_jobs, MPI_INT, OUTPUT_ROOT_PROC,
                MPI_COMM_WORLD);
    }
}
