#include <algorithm>
#include <cassert>
#include <cstring>

#include "ime/bigram.hpp"

#include "utils.hpp"

/**
 * Adds a sentence to the bigram model.
 */
void BigramIME::add_sentence(const char *sentence, size_t len) {
  Char prev = INVALID_CHAR;
  auto feed_char = [&](Char ch) {
    if (prev == ch && prev == ch_table->eos())
      return;
    if (prev != INVALID_CHAR) {
      if (bigram_freqs[prev].empty())
        bigram_freqs[prev].resize(ch_table->size());
      bigram_freqs[prev][ch]++;
    }
    unigram_freqs[ch]++;
    prev = ch;
  };

  feed_char(ch_table->sos());

  size_t i = 0;
  while (i < len) {
    size_t char_length;
    Char ch;
    // Is two-bytes character?
    if (sentence[i] & 0x80) {
      char_length = 2;
      ch = ch_table->get(sentence + i);
    } else {
      char_length = 1;
      ch = INVALID_CHAR;
    }
    if (ch == INVALID_CHAR) {
      feed_char(ch_table->eos());
    } else {
      feed_char(ch);
    }
    i += char_length;
  }
  feed_char(ch_table->eos());
}

void BigramIME::build(const SyllableTable &sy_table) {
  total = 0;
  for (Syllable s = 0; s < sy_table.size(); s++) {
    for (auto ch : ch_table->chars(s)) {
      total += unigram_freqs[ch];
    }
  }
}

std::string BigramIME::translate(const std::vector<Syllable> &syllables) const {
  struct PosState {
    double prob;
    Char prev;
  };
  std::vector<std::unordered_map<Char, PosState>> states(syllables.size() + 2);

  size_t i = 0;
  states[0][ch_table->sos()] = {1.0, INVALID_CHAR};

  auto transit = [&](const std::vector<Char> &chars) {
    for (auto &st_pa : states[i++]) {
      auto ch1 = st_pa.first;
      auto prev = st_pa.second;

      auto &bi_freqs = bigram_freqs[ch1];
      for (auto ch2 : chars) {
        u64 bi_freq = 0;
        if (!bi_freqs.empty() && (options.use_sos || ch1 != ch_table->sos())) {
          bi_freq = bi_freqs[ch2];
        }

        double prob1 = (double)bi_freq / unigram_freqs[ch1];
        double prob2 = (double)unigram_freqs[ch2] / total;
        double prob = options.lambda * prob1 + (1 - options.lambda) * prob2;

        if (!options.use_eos && ch2 == ch_table->eos())
          prob = 1.0;

        auto &state = states[i][ch2];
        if (update_max(state.prob, prev.prob * prob)) {
          state.prev = ch1;
        }
      }
    }
  };

  for (size_t i = 0; i < syllables.size(); i++) {
    transit(ch_table->chars(syllables[i]));
  }
  transit({ch_table->eos()});

  if (states[i].empty()) {
    throw std::runtime_error("No valid path found");
  }

  Char ch = ch_table->eos();
  std::vector<Char> result;
  while (true) {
    ch = states[i][ch].prev;
    if (i-- == 0) {
      break;
    }
    result.push_back(ch);
  }
  result.pop_back();

  std::string result_str;
  for (auto it = result.rbegin(); it != result.rend(); it++) {
    result_str += ch_table->utf8_char(*it);
  }
  return result_str;
}
