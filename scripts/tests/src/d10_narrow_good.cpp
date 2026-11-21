#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <cassert>
template <class To, class From>
To checked_narrow(From v) {
  static_assert(std::is_arithmetic_v<To> && std::is_arithmetic_v<From>);
  if constexpr (std::is_floating_point_v<From> && std::is_integral_v<To>) {
    const From hi = std::ldexp(From(1), std::numeric_limits<To>::digits);   // exclusive upper bound
    const From lo = std::is_signed_v<To> ? -hi : From(0);
    if (!(v >= lo && v < hi)) throw std::range_error("narrowing");          // also rejects NaN
    return static_cast<To>(v);
  } else {
    const To r = static_cast<To>(v);
    if (static_cast<From>(r) != v || ((r < To{}) != (v < From{}))) throw std::range_error("narrowing");
    return r;
  }
}
template <class To, class From> bool throws(From v){ try { (void)checked_narrow<To>(v); return false; } catch (const std::range_error&) { return true; } }
int main(){
  assert((checked_narrow<std::int16_t>(32767.0) == 32767));
  assert((checked_narrow<std::int16_t>(-32768.0) == -32768));
  assert((checked_narrow<std::int16_t>(-0.5) == 0));
  assert(throws<std::int16_t>(32768.0));
  assert(throws<std::int16_t>(-32769.0));
  assert(throws<std::int16_t>(70000.0));
  assert(throws<std::int16_t>(std::nan("")));
  assert(throws<std::int16_t>(1e300));
  assert(throws<std::int64_t>(9223372036854775808.0));        // exactly 2^63
  assert(throws<std::uint8_t>(-1.0) && throws<std::uint8_t>(256.0));
  assert(checked_narrow<std::uint8_t>(255) == 255 && throws<std::uint8_t>(256) && throws<std::uint8_t>(-1));
  assert(throws<std::int32_t>(std::int64_t{1} << 40) && throws<std::int32_t>(~0u));
  assert(!throws<std::int32_t>(std::int64_t{-5}));
}
