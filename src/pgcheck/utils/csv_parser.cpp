
#include "csv_parser.h"

PGDataTable CSVParser::parse_file(std::string csv_path) {

  PGDataTable table_res{};

  std::ifstream infile(csv_path);
  std::string line;

  std::vector <std::string> col_names = {"test","nrep","count","runtime_sec"};
  table_res.set_col_names(col_names);

  while (std::getline(infile, line)) {
    std::unordered_map <std::string, std::string> row;

    if (line[0] != '#') {
      std::istringstream iss(line);
      std::string token;
      int col_count = 0;

      while (iss >> token) {
        //std::cout << "token: " << token << std::endl;
        row[col_names[col_count]] = token;
        col_count++;
      }
    }

    table_res.add_row(row);
  }

  return table_res;
}