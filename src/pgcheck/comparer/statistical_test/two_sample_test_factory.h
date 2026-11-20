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

#ifndef SRC_PGCHECK_COMPARER_STATISTICAL_TEST_TWO_SAMPLE_TEST_FACTORY_H_
#define SRC_PGCHECK_COMPARER_STATISTICAL_TEST_TWO_SAMPLE_TEST_FACTORY_H_

#include "two_sample_test.h"
#include "ttest.h"
#include "wilcoxon_mann_whitney.h"

class TwoSampleTestFactory {
 public:
  enum TwoSampleTestType {
    TTEST = 0,
    WILCOXON_MANN_WHITNEY = 1
  };

  static constexpr auto DEFAULT_TWO_SAMPLE_TEST = TwoSampleTestType::WILCOXON_MANN_WHITNEY;

  static TwoSampleTest *create_two_sample_test(int test_id);
};

#endif  // SRC_PGCHECK_COMPARER_STATISTICAL_TEST_TWO_SAMPLE_TEST_FACTORY_H_
