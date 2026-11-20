/*  PGChecker - MPI Performance Guidelines Checker
 *
 *  Copyright 2023 Sascha Hunold, Maximilian Hagn
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

#include "two_sample_test.h"

static const double critical_values[] = {0, 6.313752, 2.919986, 2.353363, 2.131847, 2.015048, 1.943180, 1.894579,
                                         1.859548, 1.833113, 1.812461, 1.795885, 1.782288, 1.770933, 1.761310,
                                         1.753050, 1.745884, 1.739607, 1.734064, 1.729133, 1.724718, 1.720743,
                                         1.717144, 1.713872, 1.710882};
double TwoSampleTest::get_critical_value(int degrees_of_freedom) {
  if (degrees_of_freedom < 25) {
    return critical_values[degrees_of_freedom];
  } else {
    return NORMAL_CRITICAL_VALUE;
  }
}

double TwoSampleTest::get_z_value() {
  return z_value;
}

void TwoSampleTest::set_samples(const std::vector<double>& def, const std::vector<double>& alt) {
  sample_1 = def;
  sample_2 = alt;
}
