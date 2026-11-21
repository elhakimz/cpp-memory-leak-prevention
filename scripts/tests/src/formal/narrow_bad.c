#include <stdint.h>
int16_t to_i16(double d) { return (int16_t)d; }
int main(void) { double d; __CPROVER_assume(d > -1e6 && d < 1e6); (void)to_i16(d); return 0; }
