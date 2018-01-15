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

#ifndef RESULTS_OUTPUT_H_
#define RESULTS_OUTPUT_H_

#include "benchmark_job.h"
#include "reprompi_bench/sync/synchronization.h"

void print_results_header(const reprompib_options_t* opts, const reprompib_sync_module_t*  sync_module,
    const char* output_file_path, int verbose);

void print_measurement_results(FILE* f, job_t job, double* tstart_sec, double* tend_sec,
    const reprompib_sync_module_t*  sync_module,
		int verbose);

void print_summary(FILE* f, job_t job, double* tstart_sec, double* tend_sec,
    const reprompib_sync_module_t*  sync_module,
    const int print_summary_methods);

#endif /* RESULTS_OUTPUT_H_ */
