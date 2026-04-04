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
#include <string.h>
#include <getopt.h>
#include <mpi.h>

#include "sync_module_helpers.h"

static reprompib_dictionary_t params_dict;
static const int HASHTABLE_SIZE=100;
static int params_dict_initialized = 0;

enum{
  REPROMPI_ARGS_SYNC_TYPE = 1
};

void parse_sync_options(int argc, char **argv, const char* argument_name,
    sync_module_info_t* opts_p) {
    int c;
    struct option reprompi_sync_module_long_options[] = {
            { argument_name, required_argument, 0, REPROMPI_ARGS_SYNC_TYPE },
            { 0, 0, 0, 0 }
    };
    const char reprompi_sync_module_opts_str[] = "";


    opts_p->name = NULL;

    optind = 1;
    optopt = 0;
    opterr = 0; // ignore invalid options
    while (1) {

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, reprompi_sync_module_opts_str, reprompi_sync_module_long_options,
                &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case REPROMPI_ARGS_SYNC_TYPE: /* synchronization module */
            opts_p->name = strdup(optarg);
            break;
        case '?':
             break;
        }
    }
    optind = 1; // reset optind to enable option re-parsing
    opterr = 1; // reset opterr
}

void cleanup_sync_options(sync_module_info_t* opts_p) {
  if (opts_p != NULL && opts_p->name != NULL) {
    free(opts_p->name);
  }
}


int get_sync_type(const int n_types, const sync_type_t* type_list, const char* name) {
  int i;
  int type = -1;

  if (name != NULL) {
    for (i=0; i<n_types; i++) {
      if (strcmp(name, type_list[i].name) == 0) {
        type = type_list[i].type;
      }
    }
  }

  return type;
}


char* get_name_from_sync_type(const int n_types, const sync_type_t* type_list, int type) {
  int i;
  char *name = NULL;

  for (i=0; i<n_types; i++) {
    if (type_list[i].type == type) {
      name = strdup(type_list[i].name);
    }
  }
  return name;
}
