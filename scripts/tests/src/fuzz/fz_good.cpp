#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* d, size_t n){
  char buf[8] = {}; if (n > 0 && d[0] == 70) std::memcpy(buf, d, std::min(n, sizeof buf));
  return buf[0] == 0; }
