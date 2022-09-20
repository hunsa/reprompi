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

#ifndef REPROMPI_BENCHMARK_H_
#define REPROMPI_BENCHMARK_H_

#include "reprompi_bench/option_parser/parse_options.h"

typedef struct {
    long n_rep;
    const double* tstart_sec;
    const double* tend_sec;

    char* op;      // reduce method of timings over processes (min, max, mean)
    char* timertype;
    char* timername;

    char** user_svars;
    char** user_svar_names;
    int n_user_svars;

    int* user_ivars;
    char** user_ivar_names;
    int n_user_ivars;

} reprompib_job_t;


void reprompib_initialize_benchmark(int argc, char* argv[],
    reprompib_options_t *opts_p,
    reprompib_sync_module_t* clock_sync,
    reprompib_proc_sync_module_t* sync_module);

void reprompib_cleanup_benchmark(
    reprompib_options_t* opts_p,
    reprompib_sync_module_t* clock_sync,
    reprompib_proc_sync_module_t* sync_module);

void reprompib_print_bench_output(
    const reprompib_job_t* job_p,
    const reprompib_sync_module_t* clock_sync_module,
    const reprompib_proc_sync_module_t* sync_module,
    const reprompib_options_t* opts);

void reprompib_initialize_job(const long nrep, const double* tstart, const double* tend,
    const char* operation, const char* timername, const char* timertype,
    reprompib_job_t* job_p);
void reprompib_cleanup_job(reprompib_job_t* job_p);
void reprompib_add_svar_to_job(char* name, char* s, reprompib_job_t* job_p);
void reprompib_add_ivar_to_job(char* name, int v, reprompib_job_t* job_p);
int reprompib_add_parameter_to_bench(const char* key, const char* val);

#define MY_MAX(x, y) (((x) > (y)) ? (x) : (y))

void reprompi_check_and_override_lib_env_params(int *argc, char ***argv);
char **reprompi_make_argv_copy(int argc, char **argv);

#endif /* REPROMPI_BENCHMARK_H_ */
