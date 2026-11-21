#include <cstdint>
#include <cstddef>
#include <cstring>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* d, size_t n){
  char buf[8]; if (n > 8 && d[0] == 70) std::memcpy(buf, d, n);   // overflow for inputs starting 'F' longer than 8 bytes
  return buf[0] == 0; }
