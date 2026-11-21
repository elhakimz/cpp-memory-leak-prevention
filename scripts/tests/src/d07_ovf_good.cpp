#include <climits>
int main(int argc, char**){ int x = INT_MAX, r; if (__builtin_add_overflow(x, argc, &r)) return 0; return 1; }
