#include <cstdio>
int f(int argc){ int x; if (argc > 5) x = 1; return x; }
int main(int argc, char**){ if (f(argc) != 0) { std::fputs("UNINIT_VALUE_OBSERVED\n", stderr); return 1; } return 0; }
