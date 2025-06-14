
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <unordered_set>

#include "cppjieba/MixSegment.hpp"

#include "ime/word.hpp"

#include "corpus.hpp"
#include "encoding.hpp"
#include "tables.hpp"
#include "utils.hpp"

void make_dict(std::shared_ptr<SyllableTable> sy_table,
               std::shared_ptr<CharTable> ch_table,
               const std::string &dataset) {
  clock_t start = clock();

  auto word_table = std::make_shared<WordTable>();

  read_lines("extra/words_base.txt", [&](std::string &line) {
    auto index = line.find(' ');
    assert(index != std::string::npos);
    auto pinyin = sy_table->split(line.substr(index + 1));
    word_table->insert(line.substr(0, index), std::move(pinyin));
  });

  cppjieba::MixSegment seg("extra/jieba.dict.utf8", "extra/hmm_model.utf8");

  auto punctuations = load_punctuations();

  iconv_t ic = iconv_open("GBK", "UTF-8");

  std::vector<u64> uni_freqs;
  std::vector<std::unordered_map<Word, u64>> bi_freqs;
  uni_freqs.resize(word_table->size());
  bi_freqs.resize(word_table->size());

  const auto base_words_size = word_table->size();

  auto add_words = [&](const std::vector<std::string> &words) {
    Word pre = INVALID_WORD;
    auto feed_word = [&](Word word) {
      if (word != INVALID_WORD) {
        if (pre != INVALID_WORD) {
          bi_freqs[pre][word]++;
        }
        uni_freqs[word]++;
      }
      pre = word;
    };

    bool prev_is_punc = true;
    for (auto &word : words) {
      if (punctuations.count(word)) {
        if (prev_is_punc) {
          continue;
        }
        feed_word(word_table->eos());
        prev_is_punc = true;
      } else {
        auto w = word_table->get(word);
        if (w == INVALID_WORD && is_chinese(ic, *ch_table, word)) {
          w = word_table->insert(word, {});
          uni_freqs.emplace_back();
          bi_freqs.emplace_back();
        }
        if (w != INVALID_WORD) {
          if (prev_is_punc) {
            feed_word(word_table->sos());
          }
          feed_word(w);
        } else {
          pre = INVALID_WORD;
        }
        prev_is_punc = false;
      }
    }
  };

  std::vector<std::string> words;

  CorpusOptions options;
  get_dataset_options(dataset, options);
  options.progress = true;
  read_corpus(options, [&](const std::string &text) {
    words.clear();
    seg.Cut(text, words);
    add_words(words);
  });

  std::vector<Word> word_map, new_words;
  word_map.resize(word_table->size());
  new_words.resize(base_words_size);
  std::iota(word_map.begin(), word_map.begin() + base_words_size, 0);
  std::iota(new_words.begin(), new_words.end(), 0);
  for (Word i = base_words_size; i < word_table->size(); i++) {
    if (uni_freqs[i] >= 10) {
      word_map[i] = new_words.size();
      new_words.push_back(i);
    } else {
      word_map[i] = INVALID_WORD;
    }
  }

  std::ofstream words_file("extra/dict_words_" + dataset + ".txt");
  for (auto word : new_words) {
    words_file << word_table->word(word);
    auto &pinyin = word_table->pinyin(word);
    for (size_t j = 0; j < pinyin.size(); j++) {
      words_file << ' ' << sy_table->spelling(pinyin[j]);
    }
    words_file << '\n';
  }
  words_file.close();

  std::ofstream dict_file("extra/dict_" + dataset + ".bin", std::ios::binary);
  for (auto word : new_words) {
    write_uleb(dict_file, uni_freqs[word]);

    std::vector<Word> c1_words;
    std::vector<std::pair<Word, u64>> other_words;
    for (auto &p : bi_freqs[word]) {
      auto w = word_map[p.first];
      if (w == INVALID_WORD)
        continue;
      if (p.second == 1) {
        c1_words.push_back(w);
      } else {
        other_words.emplace_back(w, p.second);
      }
    }

    write_uleb(dict_file, c1_words.size());
    std::sort(c1_words.begin(), c1_words.end());
    Word last = 0;
    for (auto w : c1_words) {
      write_uleb(dict_file, w - last);
      last = w;
    }

    write_uleb(dict_file, other_words.size());
    last = 0;
    for (auto &p : other_words) {
      write_uleb(dict_file, p.first - last);
      last = p.first;
      write_uleb(dict_file, p.second);
    }
  }
  dict_file.close();

  clock_t end = clock();
  std::cerr << "Build time: " << (end - start) / (double)CLOCKS_PER_SEC
            << "s\n";
}

int main(int argc, char *argv[]) {
#ifdef ONLINE_JUDGE
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
#endif

  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " {make-dict, run} <dataset>\n";
    return 1;
  }

  auto sy_table = std::make_shared<SyllableTable>();
  auto ch_table = std::make_shared<CharTable>();
  init_tables(*sy_table, *ch_table);

  std::string dataset = argv[2];
  if (!strcmp(argv[1], "make-dict")) {
    make_dict(std::move(sy_table), std::move(ch_table), dataset);
    return 0;
  } else if (strcmp(argv[1], "run")) {
    std::cerr << "Unknown command: " << argv[1] << '\n';
    return 1;
  }

  clock_t start = clock();

  auto word_table = std::make_shared<WordTable>();

  if (!std::ifstream("extra/dict_words_" + dataset + ".txt")) {
    std::cerr << "Failed to open dict. Try running \"make-dict\" first\n";
    return 1;
  }

  auto words_path = "extra/dict_words_" + dataset + ".txt";
  read_lines(words_path.data(), [&](const std::string &line) {
    if (line == "<s>" || line == "</s>")
      return;
    auto index = line.find(' ');
    std::vector<Syllable> pinyin;
    if (index != std::string::npos) {
      pinyin = sy_table->split(line.substr(index + 1));
    }
    word_table->insert(line.substr(0, index), pinyin);
  });

  auto dict_path = "extra/dict_" + dataset + ".bin";
  WordIME ime(word_table, dict_path.data());
  // ime.options.debug = true;

  clock_t end = clock();
  std::cerr << "Load time: " << (end - start) / (double)CLOCKS_PER_SEC << "s\n";

#ifndef GRID_SEARCH
  start = clock();

  std::string line;
  while (std::getline(std::cin, line)) {
    auto syllables = sy_table->split(line);
    try {
      auto result = ime.translate(syllables);
      std::cout << result << '\n';
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << '\n';
    }
  }

  end = clock();
  std::cerr << "Translate time: " << (end - start) / (double)CLOCKS_PER_SEC
            << "s\n";
#else

  std::vector<std::vector<Syllable>> inputs;

  std::string line;
  while (std::getline(std::cin, line)) {
    auto syllables = sy_table->split(line);
    inputs.push_back(syllables);
  }

  for (int i = 1; i <= 8; i++) {
    ime.options.lambda = 1.0 - std::pow(10.0, -i);
    std::stringstream ss;
    ss << "outputs1/output_" << i << ".txt";
    std::cerr << ss.str() << '\n';
    std::ofstream out(ss.str());
    for (auto &syllables : inputs) {
      try {
        auto result = ime.translate(syllables);
        out << result << '\n';
      } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
      }
    }
  }
#endif
}
