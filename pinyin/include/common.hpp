#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#define DISABLE_COPY(Class)                                                    \
  Class(const Class &) = delete;                                               \
  Class(Class &&) = delete;                                                    \
  Class &operator=(const Class &) = delete;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/// Index type for syllables.
using Syllable = u16;
/// Index type for characters.
using Char = u16;
/// Index type for words.
using Word = u32;

const Syllable INVALID_SYLLABLE = -1;
const Char INVALID_CHAR = -1;
const Word INVALID_WORD = -1;
