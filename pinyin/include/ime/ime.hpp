#pragma once

#include "../common.hpp"

/**
 * Input Method Engine.
 */
class IME {
public:
  DISABLE_COPY(IME);

  IME() = default;

  /**
   * Translates a sequence of syllables to a UTF-8 string.
   */
  virtual std::string
  translate(const std::vector<Syllable> &syllables) const = 0;
};
