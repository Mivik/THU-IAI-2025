#include <cassert>

#include "encoding.hpp"
#include "tables.hpp"
#include "utils.hpp"

Syllable SyllableTable::insert(const std::string &key) {
  assert(!table.count(key) && "Duplicate syllable");
  spellings.push_back(key);
  return table[key] = table.size();
}
Syllable SyllableTable::get(const std::string &key) const {
  auto it = table.find(key);
  if (it != table.end()) {
    return it->second;
  }
  return INVALID_SYLLABLE;
}

std::vector<Syllable> SyllableTable::split(const std::string &seq) const {
  std::vector<Syllable> result;
  split_for_each(seq, [&](const std::string &syllable) {
    auto index = get(syllable);
    if (index == INVALID_SYLLABLE) {
      throw std::runtime_error("Invalid syllable: " + syllable);
    }
    result.push_back(index);
  });
  return result;
}

Char gbk_as_char(const char *start) {
  u8 b1 = start[0] - 0x81;
  u8 b2 = start[1] - 0x40 - (start[1] >= 0x7f);
  return ((u16)b1 * (0xfe - 0x40)) + b2;
}

Char CharTable::insert(const char *start) {
  auto ch = gbk_as_char(start);
  if (!table[ch]) {
    table[ch] = allocated++;
    utf8_chars.push_back(gbk_to_utf8(ic, start, 2));
  }
  return table[ch];
}
Char CharTable::get(const char *start) const {
  auto result = table[gbk_as_char(start)];
  return result ? result : INVALID_CHAR;
}

std::vector<Char> CharTable::chars(Syllable syllable) const {
  auto it = sy_chars.find(syllable);
  if (it != sy_chars.end()) {
    return it->second;
  }
  return {};
}

Word WordTable::insert(const std::string &key, std::vector<Syllable> pinyin) {
  assert(!table.count(key) && "Duplicate word");
  words.push_back(key);
  pinyins.push_back(std::move(pinyin));
  return table[key] = table.size() + 2;
}
Word WordTable::get(const std::string &key) const {
  auto it = table.find(key);
  if (it != table.end()) {
    return it->second;
  }
  return INVALID_WORD;
}

static std::string UNK = "<unk>";

const std::string &WordTable::word(Word word) const {
  return word == INVALID_WORD ? UNK : words[word];
}

std::vector<Syllable> WordTable::infer_pinyin(Word word) const {
  std::vector<Syllable> pinyin;
  auto &word_utf8 = this->word(word);
  for (size_t i = 0; i < word_utf8.size();) {
    char c = word_utf8[i];
    std::string ch;
    if ((c & 0x80) == 0) {
      ch = c;
      i++;
    } else if ((c & 0xE0) == 0xC0) {
      ch = word_utf8.substr(i, 2);
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {
      ch = word_utf8.substr(i, 3);
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {
      ch = word_utf8.substr(i, 4);
      i += 4;
    } else
      throw std::runtime_error("utf8_length: invalid UTF-8");

    auto ch_word = get(ch);
    if (ch_word == INVALID_WORD)
      return {};
    auto py = pinyins[ch_word];
    if (py.empty())
      return {};
    pinyin.push_back(py[0]);
  }
  return pinyin;
}
