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

#include "two_sample_test_factory.h"

TwoSampleTest *TwoSampleTestFactory::create_two_sample_test(int test_id) {
  TwoSampleTest *two_sample_test;
  switch (test_id) {
    case TwoSampleTestType::TTEST:
      two_sample_test = new TTest();
      break;
    default:
    case TwoSampleTestType::WILCOXON_MANN_WHITNEY:
      two_sample_test = new WilcoxonMannWhitney();
      break;
  }
  return two_sample_test;
}
