#include <functional>
#include <cassert>
#include <utility>
static int offs=0;
class Connection {
  std::function<void()> off_;
public:
  Connection() = default;
  explicit Connection(std::function<void()> off) : off_(std::move(off)) {}
  Connection(Connection&& o) noexcept : off_(std::exchange(o.off_, nullptr)) {}
  Connection& operator=(Connection&& o) noexcept { if (this != &o) { reset(); off_ = std::exchange(o.off_, nullptr); } return *this; }
  Connection(const Connection&) = delete; Connection& operator=(const Connection&) = delete;
  ~Connection() { reset(); }
  void reset() noexcept { if (off_) { auto f = std::exchange(off_, nullptr); f(); } }
};
int main(){
  { Connection a([]{ ++offs; }), b([]{ ++offs; });
    b = std::move(a);
  }
  assert(offs==2);
}
