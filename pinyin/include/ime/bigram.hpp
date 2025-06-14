#pragma once

#include <memory>

#include "../tables.hpp"
#include "ime.hpp"

struct BigramIMEOptions {
  /// The weight of bigram frequency
  double lambda = 0.999998;
  /// Whether to use sos (<s>).
  bool use_sos = true;
  /// Whether to use eos (</s>).
  bool use_eos = true;
};

/**
 * Bigram Input Method Engine (character-based).
 */
class BigramIME : public IME {
public:
  BigramIME(std::shared_ptr<CharTable> ch_table, BigramIMEOptions options = {})
      : options(std::move(options)), ch_table(std::move(ch_table)),
        unigram_freqs(this->ch_table->size()),
        bigram_freqs(this->ch_table->size()) {}

  /// Add sentence (GBK) to corpus
  void add_sentence(const char *sentence, size_t len);

  /// Finalize the processing of the corpus
  void build(const SyllableTable &sy_table);

  std::string translate(const std::vector<Syllable> &syllables) const override;

  BigramIMEOptions options;

private:
  std::shared_ptr<CharTable> ch_table;

  std::vector<u64> unigram_freqs;
  u64 total;

  std::vector<std::vector<u64>> bigram_freqs;
};
