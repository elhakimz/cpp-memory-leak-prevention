#include <cstdint>
int main(int argc, char**){ volatile double bias = 70000.0 * argc; return static_cast<std::int16_t>(bias) == 0; }
