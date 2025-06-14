#pragma once

#include <memory>

#include "../aho_corasick.hpp"
#include "../tables.hpp"
#include "ime.hpp"

struct PinyinMatches {
  std::vector<Word> words;
  u64 freq;
  u8 length;

  PinyinMatches(u8 length) : freq(0), length(length) {}
};

struct WordTriIMEOptions {
  /// The weight of trigram frequency
  double alpha = 0.999998;
  /// The weight of bigram frequency
  double beta = 0.9;
  /// The threshold of probability relative to the maximum.
  double filter_threshold = 1e-3;
  /// Debug mode.
  bool debug = false;
  /// Whether to use sos (<s>).
  bool use_sos = true;
  /// Whether to use eos (</s>).
  bool use_eos = true;
};

/**
 * Trigram Input Method Engine (word-based).
 */
class WordTriIME : public IME {
public:
  WordTriIME(std::shared_ptr<WordTable> word_table, const char *dict_path,
             WordTriIMEOptions options = {});

  std::string translate(const std::vector<Syllable> &syllables) const override;

  WordTriIMEOptions options;

private:
  std::shared_ptr<WordTable> word_table;
  std::vector<u64> unigram_freqs;
  u64 total;

  std::vector<std::unordered_map<Word, u32>> bigram_freqs;
  std::vector<std::unordered_map<u64, u32>> trigram_freqs;
  AhoCorasick<std::vector<Syllable>, PinyinMatches> pinyin_map;
};
