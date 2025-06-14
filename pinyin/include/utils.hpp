#pragma once

#include <fstream>
#include <iostream>
#include <unordered_set>

#include "common.hpp"

/**
 * Updates `max` if `new_val` is greater.
 *
 * Returns `true` if `max` was updated, `false` otherwise.
 */
template <class T> bool update_max(T &max, const T &new_val) {
  if (new_val > max) {
    max = new_val;
    return true;
  }
  return false;
}

/**
 * Splits a string by spaces and applies a function to each substring.
 */
template <class F> void split_for_each(const std::string &str, F &&func) {
  size_t start = 0;
  size_t end = str.find(' ');
  while (end != std::string::npos) {
    func(str.substr(start, end - start));
    start = end + 1;
    end = str.find(' ', start);
  }
  func(str.substr(start));
}

/**
 * Reads lines from a file and applies a handler to each line.
 */
template <class F> void read_lines(const char *filename, F &&handler) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file");
  }
  std::string line;
  while (std::getline(file, line)) {
    while (!line.empty() && isspace(line.back()))
      line.pop_back();
    handler(line);
  }
}

/**
 * Reads a ULEB128 encoded value from a stream.
 */
u64 read_uleb(std::istream &in);

/**
 * Writes a ULEB128 encoded value to a stream.
 */
void write_uleb(std::ostream &out, u64 value);

/**
 * Loads a set of punctuation characters from `extra/punctuations.txt`.
 */
std::unordered_set<std::string> load_punctuations();
