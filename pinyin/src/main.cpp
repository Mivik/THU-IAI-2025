#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include "ime/bigram.hpp"
#include "ime/ime.hpp"

#include "corpus.hpp"
#include "tables.hpp"

int main() {
#ifdef ONLINE_JUDGE
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
#endif

  auto sy_table = std::make_shared<SyllableTable>();
  auto ch_table = std::make_shared<CharTable>();
  init_tables(*sy_table, *ch_table);

  clock_t start = clock();

  BigramIME ime(ch_table);
  CorpusOptions options;
  get_dataset_options("sina", options);
  // We handles UTF-8 in `add_sentence`
  options.utf8 = true;
  read_corpus(options, [&](const std::string &text) {
    ime.add_sentence(text.data(), text.size());
  });

  ime.build(*sy_table);
  // ime.options.use_sos = false;
  // ime.options.use_eos = false;

  clock_t end = clock();
  std::cerr << "Build time: " << (end - start) / (double)CLOCKS_PER_SEC
            << "s\n";

  ime.options.lambda = 0.95;

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

  for (int i = 1; i < 100; i++) {
    ime.options.lambda = i / 100.;
    std::stringstream ss;
    ss << "outputs0/output_" << i << ".txt";
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
