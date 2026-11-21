#include <stdint.h>
int to_i16(double d, int16_t* out) {
  if (!(d >= -32768.0 && d < 32768.0)) return 0;     /* explicit range guard; also rejects NaN */
  *out = (int16_t)d; return 1; }
int main(void) { double d; int16_t r; __CPROVER_assume(d > -1e6 && d < 1e6); (void)to_i16(d, &r); return 0; }
