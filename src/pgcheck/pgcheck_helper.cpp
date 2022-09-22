//
// Created by Sascha on 9/22/22.
//

#include <cstdio>
//#include <iostream>
#include <memory>
#include <stdexcept>
//#include <string>
#include <array>

/**
 * not my code
 * https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
 * @param cmd
 * @return
 */

std::string exec_command(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

