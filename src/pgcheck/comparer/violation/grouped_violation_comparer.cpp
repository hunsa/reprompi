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

#include "grouped_violation_comparer.h"

static std::vector<int> col_widths = {25, 15, 5, 5, 5, 15, 10, 50, 15};

GroupedViolationComparer::GroupedViolationComparer(int test_type,
                                                   const std::string &mpi_coll_name,
                                                   int nnodes,
                                                   int ppn) :
    PGDataComparer(mpi_coll_name, nnodes, ppn), test_type(test_type) {}

PGDataTable GroupedViolationComparer::get_results() {
  std::vector <std::string> col_names = {"collective", "count", "N", "ppn", "n", "default_median", "slowdown",
                                         "fastest_mockup",
                                         "mockup_median"};
  PGDataTable res(mpi_coll_name, col_names);
  std::map<int, ComparerData> def_res;
  auto &default_data = mockup2data.at("default");

  for (auto &count : default_data->get_unique_counts()) {
    auto rts_default = default_data->get_runtimes_for_count(count);
    ComparerData default_values(rts_default, test_type);
    def_res.insert(std::make_pair(count, default_values));
  }

  for (auto &mockup_data : mockup2data) {
    if (mockup_data.first == "default") {
      continue;
    }
    auto &data = mockup2data.at(mockup_data.first);
    for (auto &count : data->get_unique_counts()) {
      auto rts = data->get_runtimes_for_count(count);
      ComparerData alt_res(rts);

      if (def_res.at(count).get_violation(alt_res)) {
        def_res.at(count).set_fastest_mockup(mockup_data.first, alt_res.get_median());
      }
    }
  }

  for (auto &count : def_res) {
    auto &data = def_res.at(count.first);
    std::unordered_map <std::string, std::string> row;
    row["collective"] = mpi_coll_name;
    row["count"] = std::to_string(count.first);
    row["N"] = std::to_string(nnodes);
    row["ppn"] = std::to_string(ppn);
    row["n"] = std::to_string(data.get_size());
    row["default_median"] = std::to_string(data.get_median_ms());

    if (data.is_violated()) {
      row["slowdown"] = std::to_string(data.get_slowdown());
      row["fastest_mockup"] = data.get_fastest_mockup();
      row["mockup_median"] = std::to_string(data.get_fastest_mockup_median_ms());
      if (has_barrier_time()) {
        // don't forget, barrier time is in 's'
        if (data.get_median_ms() - data.get_fastest_mockup_median_ms() < get_barrier_time() * 1000) {
          row["mockup"] = row["mockup"] + "*";
        }
      }
    } else {
      row["slowdown"] = "";
      row["fastest_mockup"] = "";
      row["mockup_median"] = "";
    }

    res.add_row(row);
  }
  res.set_col_widths(col_widths);
  return res;
}

