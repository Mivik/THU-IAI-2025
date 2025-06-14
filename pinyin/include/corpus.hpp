#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "encoding.hpp"
#include "tables.hpp"
#include "utils.hpp"

void init_tables(SyllableTable &sy_table, CharTable &ch_table);

/**
 * Extracts a slice from a string, using start & end pattern.
 *
 * Equivalent to `s.split(start, 1)[1].split(end, 1)[0]` in Python.

 */
std::pair<size_t, size_t> extract_slice(const std::string &str,
                                        const char *start, const char *end,
                                        size_t start_idx = 0);

/// Options for corpus reading.
struct CorpusOptions {
  /// Whether to print progress.
  bool progress = false;

  /// The keys to extract from JSON.
  std::vector<const char *> keys;

  /// The directory of the corpus.
  const char *corpus_dir = ".";
  /// The list of corpus files. Will be joined with corpus_dir.
  std::vector<const char *> corpus_list = {};

  /**
   * Whether the corpus is in UTF-8.
   *
   * If not, the corpus will be decoded from GBK before processing.
   */
  bool utf8 = true;
};

/**
 * Get options for a dataset.
 *
 * Supported datasets: `sina`, `web`.
 */
void get_dataset_options(const std::string &dataset, CorpusOptions &options);

/**
 * Read corpus.
 *
 * Corpus files should be in JSONL format. For each JSON object, strings
 * specified in options.keys will be extracted and passed to f.
 *
 * NOTE: Due to the laziness of the author, keys CANNOT be the last element of a
 *       JSON object, and the structure should be properly spaced. Specifically,
 *       your JSON line is expected to be of the form
 *
 *         {"key1": "value1", "key2": "value2", ...}
 */
template <class F> void read_corpus(const CorpusOptions &options, F &&f) {
  std::vector<std::string> patterns;
  patterns.reserve(options.keys.size());
  for (auto *key : options.keys) {
    patterns.push_back(std::string("\"") + key + "\": \"");
  }

  iconv_t ic = iconv_open("UTF-8", "GBK");

  for (auto *corpus : options.corpus_list) {
    std::string path(options.corpus_dir);
    path += corpus;
    int count = 0;
    read_lines(path.data(), [&](std::string &line) {
      for (auto &pat : patterns) {
        auto text = extract_slice(line, pat.data(), "\", ");
        const char *start = line.data() + text.first;
        const size_t length = text.second - text.first;
        if (options.utf8) {
          f(std::string(start, start + length));
        } else {
          f(gbk_to_utf8(ic, start, length));
        }
      }
      if (options.progress) {
        if (++count % 1000 == 0)
          std::cerr << "Processing " << count << '\n';
      }
    });
  }

  iconv_close(ic);
}
