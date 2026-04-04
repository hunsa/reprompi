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

#ifndef REPROMPIB_SYNC_MODULE_HELPERS_H_
#define REPROMPIB_SYNC_MODULE_HELPERS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "reprompi_bench/utils/keyvalue_store.h"

typedef struct sync_module_info {
  char* name;
} sync_module_info_t;


typedef struct sync_type {
  char* name;
  int type;
} sync_type_t;


void parse_sync_options(int argc, char **argv, const char* argument_name, sync_module_info_t* opts_p);
void cleanup_sync_options(sync_module_info_t* opts_p) ;
int get_sync_type(const int n_types, const sync_type_t* type_list, const char* name);
char* get_name_from_sync_type(const int n_types, const sync_type_t* type_list, int type);


#ifdef __cplusplus
}
#endif

#endif /* REPROMPIB_SYNC_MODULE_HELPERS_H_ */
