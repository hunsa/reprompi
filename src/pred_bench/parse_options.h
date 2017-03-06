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

#ifndef PRED_PARSE_OPTIONS_H_
#define PRED_PARSE_OPTIONS_H_

#include "reprompi_bench/option_parser/parse_common_options.h"
#include "collective_ops/collectives.h"
#include "prediction_methods/prediction_data.h"

typedef struct pred_opt {

    reprompib_common_options_t options;
    nrep_pred_params_t prediction_params; /* settings for predicting the number repetitions */

} pred_options_t;

void reprompib_free_parameters(pred_options_t* opts_p);

reprompib_error_t reprompib_parse_options(pred_options_t* opts_p, int argc, char** argv, reprompib_dictionary_t* dict);

#endif /* PRED_PARSE_OPTIONS_H_ */