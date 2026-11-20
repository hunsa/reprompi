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

#include "pgcheck_options.h"

namespace fs = std::filesystem;

bool PGCheckOptions::get_merge_coll_tables() {
  return merge_coll_tables;
}

bool PGCheckOptions::get_print_to_csv() {
  return csv;
}

bool PGCheckOptions::is_verbose() {
  return verbose;
}

bool PGCheckOptions::get_csv() {
  return csv;
}

int PGCheckOptions::get_test_type() {
  return test_type;
}

std::string PGCheckOptions::get_input_file() {
  return input_file;
}

std::string PGCheckOptions::get_output_directory() {
  return output_directory;
}

std::string PGCheckOptions::get_config_message() {
  return config_message;
}

std::vector<int> PGCheckOptions::get_comparer_list() {
  return comparer_list;
}

int PGCheckOptions::parse(int argc, char *argv[], bool is_root) {
  int c;

  struct option long_opts[] = {
      {"help",        no_argument,       NULL, 'h'},
      {"merge",       no_argument,       NULL, 'm'},
      {"csv",         no_argument,       NULL, 's'},
      {"verbose",     no_argument,       NULL, 'v'},
      {"allow-mkdir", no_argument,       NULL, 'd'},
      {"input",       required_argument, NULL, 'f'},
      {"output",      required_argument, NULL, 'o'},
      {"test",        required_argument, NULL, 't'},
      {"comp-list",   required_argument, NULL, 'c'},
      {NULL,          0,                 NULL, 0}
  };

  while ((c = getopt_long(argc, argv, ":hdmsvf:o:c:t:", long_opts, NULL)) != -1) {
    switch (c) {
      case 'h':
      case '?':
      default :
        return CONSTANTS::FAILURE;
      case 'f':
        input_file = std::string(optarg);
        break;
      case 't':
        test_type = std::atoi(optarg);
        if (test_type != TwoSampleTestFactory::TwoSampleTestType::TTEST &&
            test_type != TwoSampleTestFactory::TwoSampleTestType::WILCOXON_MANN_WHITNEY) {
          if (is_root) {
            std::cerr << "Error: invalid test type '" << optarg
                      << "', possible values are 0 (T-Test) and 1 (Wilcoxon-Mann-Whitney)" << std::endl;
          }
          return CONSTANTS::FAILURE;
        }
        break;
      case 'o':
        output_directory = std::string(optarg);
        break;
      case 'm':
        merge_coll_tables = true;
        break;
      case 's':
        csv = true;
        break;
      case 'd':
        allow_mkdir = true;
        break;
      case 'v':
        verbose = true;
        Logger::Logger::set_verbose_level(Logger::Level::VERBOSE);
        break;
      case 'c':
        std::string comp_string = optarg;
        std::string delimiter = ",";
        size_t pos;
        std::string token;

        while ((pos = comp_string.find(delimiter)) != std::string::npos) {
          token = comp_string.substr(0, pos);
          comparer_list.push_back(std::stoi(token));
          comp_string.erase(0, pos + delimiter.length());
        }

        if (!comp_string.empty()) {
          comparer_list.push_back(std::stoi(comp_string));
        }

        int min_elem = *min_element(comparer_list.begin(), comparer_list.end());
        int max_elem = *max_element(comparer_list.begin(), comparer_list.end());

        if (min_elem < 0 || max_elem >= ComparerFactory::get_number_of_comparers()) {
          if (is_root) {
            std::cerr << "Error: comparers list invalid, values should be between 0 and "
                      << ComparerFactory::get_number_of_comparers() - 1 << std::endl;
            return CONSTANTS::FAILURE;
          }
        }

        break;
    }
  }

  optind = 1;
  opterr = 1;

  // set default comparer if none is given
  if (comparer_list.size() < 1) {
    comparer_list.push_back(ComparerFactory::DEFAULT_COMPARER);
  }

  // unique and sorted comparer list if list has multiple entries
  if (comparer_list.size() > 1) {
    std::sort(comparer_list.begin(), comparer_list.end());
    comparer_list.erase(std::unique(comparer_list.begin(), comparer_list.end()), comparer_list.end());
  }

  if (is_root) {
    if (output_directory.empty()) {
      Logger::ERROR("no output directory given, use -o");
      return CONSTANTS::FAILURE;
    }

    fs::path out{output_directory};
    if (!fs::is_directory(out)) {
      if (allow_mkdir) {
        std::error_code ec;
        fs::create_directories(out, ec);
        if (ec || !fs::is_directory(out)) {
          Logger::ERROR("cannot create directory '" + output_directory + "'"
                        + (ec ? " (" + ec.message() + ")" : ""));
          return CONSTANTS::FAILURE;
        }
      } else {
        Logger::ERROR("directory '" + output_directory + "' does not exists. (use -d to force creation of output dir)");
        return CONSTANTS::FAILURE;
      }
    }
  }

  return CONSTANTS::SUCCESS;
}

std::string PGCheckOptions::get_usage_string() {
  std::stringstream usage;
  usage << "USAGE: " << "./bin/pgchecker" << " -f input_file [options]" << std::endl << std::endl;
  usage << "OPTIONS:" << std::endl;
  usage << std::setw(36) << std::left << "  ?, -h, --help" << "Display this information." << std::endl;
  usage << std::setw(36) << std::left << "  -c, --comp-list={0|1|2|3|4|5|6}"
        << "Specify the comparer type: "
        << "0=Simple | "
        << "1=Absolute Median | "
        << "2=Relative Median | "
        << "3=Violation-Test | "
        << "4=Detailed Violation-Test | "
        << "5=Grouped Violation-Test (default) | "
        << "6=Raw Data."
        << std::endl;
  usage << std::setw(36) << std::left << "  -t, --test {0|1}"
        << "Specify the test type: "
        << "0=T-Test | "
        << "1=Wilcoxon-Mann-Whitney (default)."
        << std::endl;
  usage << std::setw(36) << std::left << "  -o, --output <path>" << "Specify an existing output folder."
        << std::endl;
  usage << std::setw(36) << std::left << "  -m, --merge"
        << "Additionally results of all collectives are merged into one table." << std::endl;
  usage << std::setw(36) << std::left << "  -d, --allow-mkdir"
        << "Allow PGChecker to generate folders in the specified output folder" << std::endl;
  usage << std::setw(36) << std::left << "  -s, --csv"
        << "Print results to .csv file. Output directory must be specified. "
        << "The csv formatted table is never written to the console."
        << std::endl;
  usage << std::setw(36) << std::left << "  -v, --verbose" << "Print all information and results to console."
        << std::endl;
  return usage.str();
}
