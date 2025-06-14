#include "corpus.hpp"
#include "utils.hpp"

void init_tables(SyllableTable &sy_table, CharTable &ch_table) {
  read_lines("data/拼音汉字表.txt", [&](std::string &line) {
    auto index = line.find(' ');
    assert(index != std::string::npos);
    auto syllable = sy_table.insert(line.substr(0, index));
    split_for_each(line.substr(index + 1), [&](const std::string &ch) {
      assert(ch.length() == 2);
      ch_table.insert(syllable, ch_table.insert(ch.data()));
    });
  });
}

std::pair<size_t, size_t> extract_slice(const std::string &str,
                                        const char *start, const char *end,
                                        size_t start_idx) {
  start_idx = str.find(start, start_idx);
  assert(start_idx != std::string::npos);
  start_idx += strlen(start);
  auto end_idx = str.find(end, start_idx);
  assert(end_idx != std::string::npos);
  return std::make_pair(start_idx, end_idx);
}

void get_dataset_options(const std::string &dataset, CorpusOptions &options) {
  if (dataset == "sina") {
    options.corpus_dir = "corpus/sina_news_gbk/";
    options.corpus_list = {
        "2016-04.txt", "2016-05.txt", "2016-06.txt", "2016-07.txt",
        "2016-08.txt", "2016-09.txt", "2016-10.txt", "2016-11.txt",
    };
    options.keys = {"title", "html"};
    options.utf8 = false;
  } else if (dataset == "web") {
    options.corpus_dir = "corpus/webtext2019zh/";
    options.corpus_list = {"web_text_zh_train.json"};
    options.keys = {"title", "content"};
    options.utf8 = true;
  } else {
    throw std::runtime_error("Unknown dataset: " + dataset);
  }
}
