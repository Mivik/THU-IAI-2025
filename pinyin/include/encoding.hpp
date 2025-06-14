#pragma once

#include <iconv.h>
#include <string>

#include "tables.hpp"

std::string iconv_convert(iconv_t ic, const char *start, size_t len,
                          size_t outlen);

inline std::string gbk_to_utf8(iconv_t ic, const char *start, size_t len) {
  return iconv_convert(ic, start, len, len * 2);
}

inline std::string utf8_to_gbk(iconv_t ic, const char *start, size_t len) {
  return iconv_convert(ic, start, len, len);
}

/**
 * Check if a string is in Chinese.
 *
 * `ic` should be an iconv from UTF-8 to GBK.
 */
bool is_chinese(iconv_t ic, const CharTable &ch_table, const std::string &str);
