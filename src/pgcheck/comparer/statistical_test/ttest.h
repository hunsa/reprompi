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

#ifndef SRC_PGCHECK_COMPARER_STATISTICAL_TEST_TTEST_H_
#define SRC_PGCHECK_COMPARER_STATISTICAL_TEST_TTEST_H_

#include "two_sample_test.h"

class TTest : public TwoSampleTest {
 public:
  TTest() = default;
  /**
   * @return critical t-value for the Welch-Satterthwaite degrees of freedom
   */
  double get_critical_value() override;
  /**
   * @return Welch's t-test value for samples (unequal variances)
   */
  double get_z_value() override;
  /**
   * @return true if t-test-value is smaller than critical t-value
   */
  bool get_violation() override;
};

#endif  // SRC_PGCHECK_COMPARER_STATISTICAL_TEST_TTEST_H_
