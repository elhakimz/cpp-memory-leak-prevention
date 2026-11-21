// Template fuzz target. Replace parse() with the function that handles untrusted input.
#include <cstddef>
#include <cstdint>
#include <string>

// Example consumer: must not crash, hang, or trigger sanitizer errors for ANY byte sequence.
static bool parse(const std::string& in) { return !in.empty() && in[0] == '{'; }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  parse(std::string(reinterpret_cast<const char*>(data), size));
  return 0;   // always 0; failures are reported by the sanitizers
}
