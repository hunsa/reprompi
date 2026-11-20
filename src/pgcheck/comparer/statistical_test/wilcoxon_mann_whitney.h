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

#ifndef SRC_PGCHECK_COMPARER_STATISTICAL_TEST_WILCOXON_MANN_WHITNEY_H_
#define SRC_PGCHECK_COMPARER_STATISTICAL_TEST_WILCOXON_MANN_WHITNEY_H_

#include <algorithm>
#include <vector>
#include <utility>

#include "two_sample_test.h"

struct rank_element {
  double runtime;
  int sample_id;

  bool operator<(const rank_element &a) const {
    return runtime < a.runtime;
  }
};

class WilcoxonMannWhitney : public TwoSampleTest {
 private:
  double get_shared_rank_deviation(std::size_t shared_rank_count);
  double get_expected_value();

 public:
  WilcoxonMannWhitney() = default;
  /**
   * @return one-sided critical value of the standard normal distribution (alpha = 0.05)
   */
  double get_critical_value() override;
  /**
   * @return wilcoxon-mann-whitney-test value for samples
   */
  double get_z_value() override;
  /**
   * @return true if wilcoxon-mann-whitney-test value is smaller than critical z-value
   */
  bool get_violation() override;
};

#endif  // SRC_PGCHECK_COMPARER_STATISTICAL_TEST_WILCOXON_MANN_WHITNEY_H_
