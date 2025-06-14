#pragma once

#include <iconv.h>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "common.hpp"

/**
 * Table mapping syllables into indices.
 */
class SyllableTable {
public:
  DISABLE_COPY(SyllableTable);

  SyllableTable() = default;

  /**
   * Inserts a syllable string into the table.
   *
   * Returns the index of the inserted syllable.
   */
  Syllable insert(const std::string &key);
  /**
   * Retrieves the index of a syllable string.
   *
   * Returns `INVALID_SYLLABLE` if the syllable is not in the table.
   */
  Syllable get(const std::string &key) const;

  /**
   * Retrieves the syllable string corresponding to an index.
   */
  const std::string &spelling(Syllable syllable) const {
    return spellings[syllable];
  }

  size_t size() const { return table.size(); }

  /**
   * Splits a sequence of syllable strings into a vector of syllables.
   *
   * E.g. "zhong guo" -> [0, 1]
   */
  std::vector<Syllable> split(const std::string &seq) const;

private:
  std::unordered_map<std::string, Syllable> table;
  std::vector<std::string> spellings;
};

const size_t GBK_CHAR_COUNT = 23940;

/**
 * Table mapping syllable into GBK characters, character into indices.
 */
class CharTable {
public:
  DISABLE_COPY(CharTable);

  CharTable()
      : ic(iconv_open("UTF-8", "GBK")), allocated(2), utf8_chars({"", ""}) {}

  ~CharTable() { iconv_close(ic); }

  /**
   * Inserts a character string into the table.
   *
   * Returns the index of the inserted character.
   *
   * NOTE: The input string must be a valid GBK character.
   */
  Char insert(const char *start);
  /**
   * Retrieves the index of a character string.
   *
   * Returns `INVALID_CHAR` if the character is not in the table.
   */
  Char get(const char *start) const;

  /**
   * Inserts a syllable-character pair into the table.
   */
  void insert(Syllable syllable, Char ch) { sy_chars[syllable].push_back(ch); }

  /// Start of sentence character.
  Char sos() const { return 0; }
  /// End of sentence character.
  Char eos() const { return 1; }

  size_t size() const { return allocated; }

  /**
   * Retrieves the list of characters corresponding to a syllable.
   */
  std::vector<Char> chars(Syllable syllable) const;

  /**
   * Retrieves the UTF-8 character string corresponding to an index.
   */
  const std::string &utf8_char(Char ch) const { return utf8_chars[ch]; }

private:
  iconv_t ic;
  Char table[GBK_CHAR_COUNT], allocated;
  std::vector<std::string> utf8_chars;
  std::unordered_map<Syllable, std::vector<Char>> sy_chars;
};

/**
 * Table mapping word into indices & pinyin, indices into words.
 */
class WordTable {
public:
  DISABLE_COPY(WordTable);

  WordTable() : words({"<s>", "</s>"}), pinyins({{}, {}}) {}

  /// Start of sentence word (<s>).
  Word sos() const { return 0; }
  /// End of sentence word (</s>).
  Word eos() const { return 1; }

  /**
   * Inserts a word string and its syllable sequence (pinyin) into the table.
   *
   * Returns the index of the inserted word.
   */
  Word insert(const std::string &key, std::vector<Syllable> pinyin);
  /**
   * Retrieves the index of a word string.
   *
   * Returns `INVALID_WORD` if the word is not in the table.
   */
  Word get(const std::string &key) const;

  size_t size() const { return table.size() + 2; }

  /**
   * Retrieves the word string corresponding to an index.
   *
   * This handles `INVALID_WORD` as well (returns <unk>).
   */
  const std::string &word(Word word) const;
  const std::vector<Syllable> &pinyin(Word word) const { return pinyins[word]; }

  /**
   * Infer the syllable sequence (pinyin) of a word.
   *
   * Sometimes a word's pinyin is not explicitly given for the sake of saving
   * space. We can infer it from its characters and the characters' pinyins.
   */
  std::vector<Syllable> infer_pinyin(Word word) const;

private:
  std::unordered_map<std::string, Word> table;
  std::vector<std::string> words;
  std::vector<std::vector<Syllable>> pinyins;
};
