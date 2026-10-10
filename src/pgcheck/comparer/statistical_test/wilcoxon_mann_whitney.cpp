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

#include "wilcoxon_mann_whitney.h"

double WilcoxonMannWhitney::get_z_value() {
  double n = sample_1.size();
  double m = sample_2.size();
  double size_squared = n * m;
  double size_sum = n + m;
  double sample_1_rank_sum = 0;
  double sample_2_rank_sum = 0;

  std::vector <rank_element> ranks;

  for (auto iter = sample_1.begin(); iter != sample_1.end(); ++iter) {
    ranks.push_back({*iter, 1});
  }

  for (auto iter = sample_2.begin(); iter != sample_2.end(); ++iter) {
    ranks.push_back({*iter, 2});
  }

  std::sort(ranks.begin(), ranks.end());

  // assign 1-based ranks; tied runtimes share the average of their ranks
  const size_t N = ranks.size();
  size_t i = 0;
  while (i < N) {
    size_t j = i;
    while (j + 1 < N && ranks[j + 1].runtime == ranks[i].runtime) {
      ++j;
    }
    double mean_rank = (i + 1 + j + 1) / 2.0;
    for (size_t k = i; k <= j; ++k) {
      if (ranks[k].sample_id == 1) {
        sample_1_rank_sum += mean_rank;
      } else {
        sample_2_rank_sum += mean_rank;
      }
    }
    i = j + 1;
  }

  // assert that all ranks where assigned
  assert((sample_1_rank_sum + sample_2_rank_sum) == (size_sum * (size_sum + 1) / 2));

  // assert that u calculation was successful
  double U = size_squared + n * (n + 1) / 2 - sample_1_rank_sum;
  assert((U + size_squared + m * (m + 1) / 2 - sample_2_rank_sum) == size_squared);

  double standard_deviation = std::sqrt(size_squared * (size_sum + 1) / 12);
  z_value = (U - get_expected_value()) / standard_deviation;
  return z_value;
}

double WilcoxonMannWhitney::get_critical_value() {
  int size = std::min(sample_1.size(), sample_2.size());
  return TwoSampleTest::get_critical_value(size);
}

double WilcoxonMannWhitney::get_shared_rank_deviation(int shared_rank_count) {
  return (std::pow(shared_rank_count, 3) - shared_rank_count) / 2;
}

double WilcoxonMannWhitney::get_expected_value() {
  return static_cast<double>(sample_1.size()) * sample_2.size() / 2;
}

bool WilcoxonMannWhitney::get_violation() {
  if (z_value != 0) {
    return z_value < -get_critical_value();
  } else {
    return get_z_value() < -get_critical_value();
  }
}
