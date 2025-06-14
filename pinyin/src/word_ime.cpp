#include <algorithm>
#include <cassert>
#include <cmath>

#ifdef KN_SMOOTHING
#include <cstring>
#endif

#include "ime/word.hpp"

#include "utils.hpp"

WordIME::WordIME(std::shared_ptr<WordTable> wt, const char *dict_path,
                 WordIMEOptions options)
    : options(std::move(options)), word_table(std::move(wt)) {
  unigram_freqs.resize(word_table->size());
  bigram_freqs.resize(word_table->size());
  total = 0;

  std::ifstream dict_file(dict_path);
  for (Word i = 0; i < word_table->size(); i++) {
    unigram_freqs[i] = read_uleb(dict_file);
    total += unigram_freqs[i];

    u64 c1_size = read_uleb(dict_file);
    Word cur = 0;
    for (u64 j = 0; j < c1_size; j++) {
      cur += read_uleb(dict_file);
      bigram_freqs[i][cur] = 1;
    }

    u64 other_size = read_uleb(dict_file);
    cur = 0;
    for (u64 j = 0; j < other_size; j++) {
      cur += read_uleb(dict_file);
      bigram_freqs[i][cur] = read_uleb(dict_file);
    }
  }
  assert(dict_file.peek() == EOF);
  dict_file.close();

  for (Word word = 2; word < word_table->size(); word++) {
    auto pinyin = word_table->pinyin(word);
    if (pinyin.empty())
      pinyin = word_table->infer_pinyin(word);
    if (pinyin.empty())
      continue;

    auto &ptr = pinyin_map.insert(pinyin);
    if (!ptr) {
      ptr = std::unique_ptr<PinyinMatches>(new PinyinMatches(pinyin.size()));
    }
    ptr->words.push_back(word);
    ptr->freq += unigram_freqs[word];
  }

  pinyin_map.build();

#ifdef KN_SMOOTHING
  memset(t, 0, sizeof(t));
  memset(D, 0, sizeof(D));

  // u2 && t[1]
  u2.resize(word_table->size());
  for (Word word = 0; word < word_table->size(); word++) {
    for (auto &pa : bigram_freqs[word]) {
      assert(pa.second);
      if (pa.second <= 4) {
        t[1][pa.second - 1]++;
      }
      u2[pa.first]++;
    }
  }
  u2[word_table->sos()] = unigram_freqs[word_table->sos()];

  // t[0] & ud
  ud = 0;
  for (Word word = 0; word < word_table->size(); word++) {
    if (u2[word] && u2[word] <= 4) {
      t[0][u2[word] - 1]++;
    }
    ud += u2[word];
  }
  ud -= u2[word_table->sos()];

  // D
  for (size_t n = 0; n < 2; n++) {
    for (size_t k = 0; k < 3; k++) {
      D[n][k] = k + 1 -
                (double)((k + 2) * t[n][0] * t[n][k + 1]) / t[n][k] /
                    (t[n][0] + 2 * t[n][1]);
    }
  }

  // beps
  double beps = 0;
  for (size_t i = 0; i < 3; i++) {
    beps += D[0][i] * t[0][i];
  }
  beps /= ud;

  u64 size = 0;
  for (Word word = 0; word < word_table->size(); word++) {
    if (u2[word])
      size++;
  }

  // b
  b.resize(word_table->size());
  p.resize(word_table->size());
  for (Word word = 0; word < word_table->size(); word++) {
    if (!u2[word])
      continue;
    u64 counts[3] = {0, 0, 0};
    for (auto &pa : bigram_freqs[word]) {
      if (pa.second <= 3) {
        counts[pa.second - 1]++;
      }
    }
    for (size_t i = 0; i < 3; i++) {
      b[word] += D[1][i] * counts[i];
    }
    b[word] /= unigram_freqs[word];
    double u = (u2[word] - D[0][std::min((u64)3, u2[word]) - 1]) / ud;
    p[word] = u + beps / size;
  }
#endif
}

std::string WordIME::translate(const std::vector<Syllable> &syllables) const {
  struct PosState {
    double prob;
    Word prev;
    u8 length;
  };
  std::vector<std::unordered_map<Word, PosState>> states(syllables.size() + 2);

  size_t i = 0;
  states[0][word_table->sos()] = {1.0, INVALID_WORD, 0};

  auto transit = [&](const std::vector<Word> &words, u64 sy_freq, u8 length) {
    if (i < length)
      return;
    for (auto &st_pa : states[i - length]) {
      auto word1 = st_pa.first;
      auto prev = st_pa.second;

      auto &bi_freqs = bigram_freqs[word1];
      for (auto word2 : words) {
        u64 bi_freq = 0;
        if (options.use_sos || word1 != word_table->sos()) {
          auto it = bi_freqs.find(word2);
          if (it != bi_freqs.end()) {
            bi_freq = it->second;
          }
        }

#ifdef KN_SMOOTHING
        double u = std::max(bi_freq - D[1][std::min((u64)3, bi_freq) - 1], 0.) /
                   unigram_freqs[word1];
        double prob = u + b[word1] * p[word2];
#else
        double prob1 = (double)bi_freq / unigram_freqs[word1];
        double prob2 = sy_freq ? (double)unigram_freqs[word2] / sy_freq : 0;
        double prob = options.lambda * prob1 + (1 - options.lambda) * prob2;

        if (!options.use_eos && word2 == word_table->eos())
          prob = 1.0;
#endif

        if (options.debug) {
          std::cerr << "> " << word_table->word(word1) << ' '
                    << word_table->word(word2) << ' ' << prob << '\n';
        }

        auto &state = states[i][word2];
        if (update_max(state.prob, prev.prob * prob)) {
          state.prev = word1;
          state.length = length;
        }
      }
    }
  };

  u32 node = 0;
  for (size_t j = 0; j < syllables.size(); j++) {
    i++;
    node = pinyin_map.transit(node, syllables[j]);
    assert(node != INVALID_NODE);
    pinyin_map.for_all_values(
        node, [&](const std::unique_ptr<PinyinMatches> &matches) {
          transit(matches->words, matches->freq, matches->length);
        });

    if (options.debug) {
      std::vector<std::pair<Word, PosState>> new_states;
      for (auto &st_pa : states[i]) {
        new_states.push_back(st_pa);
      }
      std::sort(new_states.begin(), new_states.end(),
                [](const std::pair<Word, PosState> &a,
                   const std::pair<Word, PosState> &b) {
                  return a.second.prob > b.second.prob;
                });
      for (size_t j = 0; j < std::min((size_t)20, new_states.size()); j++) {
        std::cerr << word_table->word(new_states[j].second.prev) << ' '
                  << word_table->word(new_states[j].first) << ": "
                  << new_states[j].second.prob << '\n';
      }
      std::cerr << '\n';
    }
  }
  i++;
  transit({word_table->eos()}, unigram_freqs[word_table->eos()], 1);

  if (states[i].empty()) {
    throw std::runtime_error("No valid path found");
  }

  Word word = word_table->eos();
  std::vector<Word> result;
  while (true) {
    auto &state = states[i][word];
    word = state.prev;
    i -= state.length;
    result.push_back(word);
    if (i == 0) {
      break;
    }
  }
  result.pop_back();

  std::string result_str;
  for (auto it = result.rbegin(); it != result.rend(); it++) {
    result_str += word_table->word(*it);
  }
  return result_str;
}