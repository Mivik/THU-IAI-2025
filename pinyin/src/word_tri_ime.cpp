#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>

#include "ime/word_tri.hpp"

#include "utils.hpp"

WordTriIME::WordTriIME(std::shared_ptr<WordTable> wt, const char *dict_path,
                       WordTriIMEOptions options)
    : options(std::move(options)), word_table(std::move(wt)) {
  unigram_freqs.resize(word_table->size());
  bigram_freqs.resize(word_table->size());
  trigram_freqs.resize(word_table->size());
  total = 0;

  std::ifstream dict_file(dict_path, std::ios::binary);
  for (Word i = 0; i < word_table->size(); i++) {
    unigram_freqs[i] = read_uleb(dict_file);
    total += unigram_freqs[i];

    u64 c1_size = read_uleb(dict_file);
    Word last = 0;
    for (u64 j = 0; j < c1_size; j++) {
      last += read_uleb(dict_file);
      Word w = read_uleb(dict_file);
      bigram_freqs[i][last] = 1;
      if (w != word_table->sos())
        trigram_freqs[i][((u64)last << 32) | w] = 1;
    }

    u64 other_size = read_uleb(dict_file);
    last = 0;
    for (u64 j = 0; j < other_size; j++) {
      last += read_uleb(dict_file);
      auto &bi_freq = bigram_freqs[i][last];
      bi_freq = read_uleb(dict_file);

      u64 c1_tri_size = read_uleb(dict_file);
      Word last2 = 0;
      for (u64 k = 0; k < c1_tri_size; k++) {
        last2 += read_uleb(dict_file);
        bi_freq += trigram_freqs[i][((u64)last << 32) | last2] = 1;
      }

      u64 other_tri_size = read_uleb(dict_file);
      last2 = 0;
      for (u64 k = 0; k < other_tri_size; k++) {
        last2 += read_uleb(dict_file);
        bi_freq += trigram_freqs[i][((u64)last << 32) | last2] =
            read_uleb(dict_file);
      }
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
}

std::string
WordTriIME::translate(const std::vector<Syllable> &syllables) const {
  struct PosState {
    double prob;
    Word prev;
    u8 length;
  };
  std::vector<std::map<std::pair<Word, Word>, PosState>> states(
      syllables.size() + 2);

  size_t i = 0;
  states[0][{INVALID_WORD, word_table->sos()}] = {1.0, INVALID_WORD, 0};
  double layer_max_prob = 0.;

  auto transit = [&](const std::vector<Word> &words, u64 sy_freq, u8 length) {
    if (i < length)
      return;
    for (auto &st_pa : states[i - length]) {
      auto word1 = st_pa.first.first;
      auto word2 = st_pa.first.second;
      auto prev = st_pa.second;

      auto &bi_freqs = bigram_freqs[word2];
      for (auto word3 : words) {
        u64 bi_freq = 0, bi2_freq = 0, tri_freq = 0;

        if (options.use_sos || word2 != word_table->sos()) {
          auto it = bi_freqs.find(word3);
          if (it != bi_freqs.end()) {
            bi_freq = it->second;
          }

          if (word1 != INVALID_WORD &&
              (options.use_sos || word1 != word_table->sos())) {
            auto it2 = trigram_freqs[word1].find(((u64)word2 << 32) | word3);
            if (it2 != trigram_freqs[word1].end()) {
              tri_freq = it2->second;
            }

            auto it3 = bigram_freqs[word1].find(word2);
            if (it3 != bigram_freqs[word1].end()) {
              bi2_freq = it3->second;
            }
          }
        }

        double prob1 = bi2_freq ? ((double)tri_freq / bi2_freq) : 0.;
        double prob2 = (double)bi_freq / unigram_freqs[word2];
        double prob3 = (double)unigram_freqs[word3] / sy_freq;

        double prob = options.beta * prob1 +
                      (1 - options.beta) *
                          (options.alpha * prob2 + (1 - options.alpha) * prob3);

        if (!options.use_eos && word3 == word_table->eos())
          prob = 1.0;

        if (options.debug) {
          std::cerr << "> " << word_table->word(word1) << ' '
                    << word_table->word(word2) << ' ' << word_table->word(word3)
                    << ' ' << prob << '\n';
        }

        auto &state = states[i][{word2, word3}];
        if (update_max(state.prob, prev.prob * prob)) {
          state.prev = word1;
          state.length = length;
          update_max(layer_max_prob, state.prob);
        }
      }
    }
  };

  u32 node = 0;
  for (size_t j = 0; j < syllables.size(); j++) {
    layer_max_prob = 0.;
    i++;
    node = pinyin_map.transit(node, syllables[j]);
    assert(node != INVALID_NODE);
    pinyin_map.for_all_values(
        node, [&](const std::unique_ptr<PinyinMatches> &matches) {
          transit(matches->words, matches->freq, matches->length);
        });

    // Filter out low-probability states
    double threshold = layer_max_prob * options.filter_threshold;
    for (auto it = states[i].begin(); it != states[i].end();) {
      if (it->second.prob < threshold) {
        it = states[i].erase(it);
      } else {
        ++it;
      }
    }

    if (options.debug) {
      std::vector<std::pair<std::pair<Word, Word>, PosState>> new_states;
      for (auto &st_pa : states[i]) {
        new_states.push_back(st_pa);
      }
      std::sort(new_states.begin(), new_states.end(),
                [](const std::pair<std::pair<Word, Word>, PosState> &a,
                   const std::pair<std::pair<Word, Word>, PosState> &b) {
                  return a.second.prob > b.second.prob;
                });
      for (size_t j = 0; j < std::min((size_t)20, new_states.size()); j++) {
        std::cerr << word_table->word(new_states[j].second.prev) << ' '
                  << word_table->word(new_states[j].first.first) << ' '
                  << word_table->word(new_states[j].first.second) << ": "
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

  Word word1 = INVALID_WORD, word2 = word_table->eos();
  double max_prob = 0.;
  for (auto &state : states[i]) {
    if (state.second.prob > max_prob) {
      word1 = state.first.first;
      max_prob = state.second.prob;
    }
  }
  if (word1 == INVALID_WORD) {
    throw std::runtime_error("No valid path found");
  }

  std::vector<Word> result = {word1};
  while (true) {
    auto &state = states[i][{word1, word2}];
    word2 = word1;
    word1 = state.prev;
    i -= state.length;
    if (i == 0) {
      break;
    }
    result.push_back(word1);
  }
  result.pop_back();

  std::string result_str;
  for (auto it = result.rbegin(); it != result.rend(); it++) {
    result_str += word_table->word(*it);
  }
  return result_str;
}