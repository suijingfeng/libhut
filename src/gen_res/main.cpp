#include <algorithm>
#include <chrono>
#include <cstring>
#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "hut/utils.hpp"

using namespace std;
using namespace std::experimental::filesystem::v1;
using namespace std::chrono;
using namespace hut;

constexpr auto line_size = 0x10;

int main(int argc, char **argv) {
  auto start = steady_clock::now();

  if (argc <= 3)
    throw runtime_error(sstream("usage: ") << argv[0] << " <namespace> <output> <list of input...>");

  path output_path = argv[2];
  if (!exists(output_path.parent_path()))
    create_directories(output_path.parent_path());

  ofstream output(output_path, ios::out | ios::trunc);
  if (!output.is_open())
    throw runtime_error(sstream("can't open ") << output_path << ": " << strerror(errno));

  output << "// This is an autogenerated file.\n";
  output << "#pragma once\n";
  output << "#include <array>\n";
  output << "namespace hut {\n";
  output << "namespace " << argv[1] << " {\n";

  for (auto i = 3; i < argc; i++) {
    path input_path = argv[i];

    ifstream input(input_path, ios::ate | ios::binary);
    if (!input.is_open())
      throw runtime_error(sstream("can't open ") << input_path << ": " << strerror(errno));

    string symbol = input_path.filename().string();
    std::replace(symbol.begin(), symbol.end(), '.', '_');

    auto found_size = input.tellg();
    auto written = 0;

    output << "const std::array<uint8_t, " << dec << input.tellg() << "> " << symbol << " = {\n";
    input.seekg(0);
    while (!input.eof()) {
      uint8_t line[line_size];
      input.read((char *)line, line_size);
      output << "\t/*" << hex << setw(6) << setfill('0') << written << "*/ ";
      for (auto c = 0; c < input.gcount(); c++)
        output << "0x" << setw(2) << (uint32_t)line[c] << ", ";
      written += input.gcount();
      output << "\n";
    }
    output << "}; // " << symbol << "\n\n";
  }

  output << "}  // namespace " << argv[1] << '\n';
  output << "}  // namespace hut" << std::endl;

  std::cout << "Generated " << output_path << " in "
            << duration<double, std::milli>(steady_clock::now() - start).count() << "ms." << std::endl;
}