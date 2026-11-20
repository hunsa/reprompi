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

#include "ttest.h"

// Welch's t-test: does not assume equal variances of both samples
// (run-time distributions of different algorithms usually have different variances,
// and sample sizes differ when measurements are time-limited)

double TTest::get_critical_value() {
  const double n = static_cast<double>(sample_1.size());
  const double m = static_cast<double>(sample_2.size());
  if (n < 2 || m < 2) {
    return TwoSampleTest::get_critical_value(1);
  }

  StatisticsUtils<double> statistics_utils;
  const double se_1 = statistics_utils.variance(sample_1) / n;
  const double se_2 = statistics_utils.variance(sample_2) / m;

  // Welch-Satterthwaite degrees of freedom
  double df = n + m - 2;
  const double denominator = se_1 * se_1 / (n - 1) + se_2 * se_2 / (m - 1);
  if (denominator > 0) {
    df = (se_1 + se_2) * (se_1 + se_2) / denominator;
  }

  // round down (conservative: larger critical value), at least 1 degree of freedom
  const int df_int = std::max(1, static_cast<int>(std::floor(df)));
  return TwoSampleTest::get_critical_value(df_int);
}

double TTest::get_z_value() {
  const double n = static_cast<double>(sample_1.size());
  const double m = static_cast<double>(sample_2.size());
  if (n < 2 || m < 2) {
    // variance is undefined: no evidence for a difference
    z_value = 0;
    return z_value;
  }

  StatisticsUtils<double> statistics_utils;
  const double standard_error = std::sqrt(statistics_utils.variance(sample_1) / n +
                                          statistics_utils.variance(sample_2) / m);
  const double mean_diff = statistics_utils.mean(sample_2) - statistics_utils.mean(sample_1);
  if (standard_error <= 0) {
    // both samples are constant: identical values give no evidence for a difference,
    // different values are a certain difference
    z_value = (mean_diff == 0) ? 0 : std::copysign(INFINITY, mean_diff);
    return z_value;
  }

  z_value = mean_diff / standard_error;
  return z_value;
}

bool TTest::get_violation() {
  return get_z_value() < -get_critical_value();
}
