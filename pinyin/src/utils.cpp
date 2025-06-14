#include <fstream>

#include "utils.hpp"

u64 read_uleb(std::istream &in) {
  u64 value = 0;
  u8 shift = 0, byte;
  while (true) {
    in.read((char *)&byte, 1);
    value |= (u64)(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      break;
    }
    shift += 7;
  }
  return value;
}

void write_uleb(std::ostream &out, u64 value) {
  while (value >= 0x80) {
    char byte = (char)((value & 0x7f) | 0x80);
    out.write(&byte, 1);
    value >>= 7;
  }
  char byte = (char)value;
  out.write(&byte, 1);
}

std::unordered_set<std::string> load_punctuations() {
  std::unordered_set<std::string> result;
  std::ifstream file("extra/punctuations.txt");
  std::string line;
  while (std::getline(file, line)) {
    result.insert(line);
  }
  return result;
}
