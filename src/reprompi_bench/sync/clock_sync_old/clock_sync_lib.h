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
#ifndef REPROMPIB_CLOCK_SYNCHRONIZATION_LIB_H_
#define REPROMPIB_CLOCK_SYNCHRONIZATION_LIB_H_

#include "synchronization.h"

#ifdef __cplusplus
extern "C" {
#endif

void register_hca_module(reprompib_sync_module_t *sync_mod);
void register_hca2_module(reprompib_sync_module_t *sync_mod);
void register_skampi_module(reprompib_sync_module_t *sync_mod);
void register_jk_module(reprompib_sync_module_t *sync_mod);
void register_hca3_module(reprompib_sync_module_t *sync_mod);
void register_hca3_offset_module(reprompib_sync_module_t *sync_mod);
void register_topo_aware_sync1_module(reprompib_sync_module_t *sync_mod);
void register_topo_aware_sync2_module(reprompib_sync_module_t *sync_mod);
void register_no_clock_sync_module(reprompib_sync_module_t *sync_mod);

#ifdef __cplusplus
}
#endif

#endif /* REPROMPIB_CLOCK_SYNCHRONIZATION_LIB_H_ */
