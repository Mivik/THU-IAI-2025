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

struct WordIMEOptions {
  /// The weight of bigram frequency
  double lambda = 0.999998;
  /// Debug mode.
  bool debug = false;
  /// Whether to use sos (<s>).
  bool use_sos = true;
  /// Whether to use eos (</s>).
  bool use_eos = true;
};

/**
 * Bigram Input Method Engine (word-based).
 */
class WordIME : public IME {
public:
  WordIME(std::shared_ptr<WordTable> word_table, const char *dict_path,
          WordIMEOptions options = {});

  std::string translate(const std::vector<Syllable> &syllables) const override;

  WordIMEOptions options;

private:
  std::shared_ptr<WordTable> word_table;
  std::vector<u64> unigram_freqs;
  u64 total;

#ifdef KN_SMOOTHING
  std::vector<u64> u2;
  std::vector<double> b, p;
  u64 ud;
  u64 t[2][4];
  double D[2][3];
#endif

  std::vector<std::unordered_map<Word, u64>> bigram_freqs;
  AhoCorasick<std::vector<Syllable>, PinyinMatches> pinyin_map;
};
