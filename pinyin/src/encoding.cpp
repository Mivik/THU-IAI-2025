#include "encoding.hpp"

std::string iconv_convert(iconv_t ic, const char *start, size_t len,
                          size_t outlen) {
  std::string result;
  result.resize(outlen);

  char *inbuf = const_cast<char *>(start);
  size_t inbytesleft = len;
  char *outbuf = const_cast<char *>(result.data());
  size_t outbytesleft = result.size();
  iconv(ic, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
  result.resize(result.size() - outbytesleft);
  return result;
}


bool is_chinese(iconv_t ic, const CharTable &ch_table, const std::string &str) {
  auto gbk = utf8_to_gbk(ic, str.data(), str.size());
  if (gbk.size() & 1)
    return false;
  for (size_t i = 0; i < gbk.size(); i += 2) {
    if ((gbk[i] & 0x80) == 0)
      return false;
    if (ch_table.get(gbk.data() + i) == INVALID_CHAR)
      return false;
  }
  return true;
}