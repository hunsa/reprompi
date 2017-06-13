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

#ifndef BARRIER_SYNC_H_
#define BARRIER_SYNC_H_

#include "reprompi_bench/sync/option_parser/sync_parse_options.h"

void mpibarrier_init_synchronization_module(const reprompib_sync_options_t parsed_opts, const long nrep);
void mpibarrier_parse_options(int argc, char **argv, reprompib_sync_options_t* opts_p);
void mpibarrier_init_synchronization(void);
void mpibarrier_start_synchronization(void);
void mpibarrier_stop_synchronization(void);
void mpibarrier_cleanup_synchronization_module(void);

int* mpibarrier_get_local_sync_errorcodes(void);

double mpibarrier_get_normalized_time(double local_time);

void mpibarrier_print_sync_parameters(FILE* f);

#endif /* BARRIER_SYNC_H_ */
