#ifndef CANTERA_COLUMNFILE_INTERNAL_H_
#define CANTERA_COLUMNFILE_INTERNAL_H_ 1

#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <experimental/string_view>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <kj/common.h>
#include <kj/debug.h>

#include "columnfile.h"

namespace cantera {
namespace columnfile_internal {

// The magic code string is designed to cause a parse error if someone attempts
// to parse the file as a CSV.
static const char kMagic[4] = {'\n', '\t', '\"', 0};

enum Codes : uint8_t {
  kCodeNull = 0xff,
};

inline uint32_t GetUInt(std::experimental::string_view& input) {
  auto begin = input.data();
  auto i = begin;
  uint32_t b = *i++;
  uint32_t result = b & 127;
  if (b < 0x80) goto done;
  b = *i++;
  result |= (b & 127) << 6;
  if (b < 0x80) goto done;
  b = *i++;
  result |= (b & 127) << 13;
  if (b < 0x80) goto done;
  b = *i++;
  result |= (b & 127) << 20;
  if (b < 0x80) goto done;
  b = *i++;
  KJ_REQUIRE(b <= 0x1f, b);
  result |= b << 27;
done:
  input.remove_prefix(i - begin);
  return result;
}

inline int32_t GetInt(std::experimental::string_view& input) {
  const auto u = GetUInt(input);
  return (u >> 1) ^ -((int32_t)(u & 1));
}

inline void PutUInt(std::string& output, uint32_t value) {
  if (value < (1 << 7)) {
    output.push_back(value);
  } else if (value < (1 << 13)) {
    output.push_back((value & 0x3f) | 0x80);
    output.push_back(value >> 6);
  } else if (value < (1 << 20)) {
    output.push_back((value & 0x3f) | 0x80);
    output.push_back((value >> 6) | 0x80);
    output.push_back(value >> 13);
  } else if (value < (1 << 27)) {
    output.push_back((value & 0x3f) | 0x80);
    output.push_back((value >> 6) | 0x80);
    output.push_back((value >> 13) | 0x80);
    output.push_back(value >> 20);
  } else {
    output.push_back((value & 0x3f) | 0x80);
    output.push_back((value >> 6) | 0x80);
    output.push_back((value >> 13) | 0x80);
    output.push_back((value >> 20) | 0x80);
    output.push_back(value >> 27);
  }
}

inline void PutInt(std::string& output, int32_t value) {
  static const auto sign_shift = sizeof(value) * 8 - 1;

  PutUInt(output, (value << 1) ^ (value >> sign_shift));
}

void CompressZLIB(std::string& output, const string_view& input);

}  // namespace columnfile_internal
}  // namespace cantera

#endif  // !CANTERA_COLUMNFILE_INTERNAL_H_
