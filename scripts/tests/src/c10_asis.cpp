#include <functional>
#include <cassert>
#include <utility>
static int offs=0;
class Connection {
  std::function<void()> off_;
public:
  explicit Connection(std::function<void()> off) : off_(std::move(off)) {}
  Connection(Connection&&) = default; Connection& operator=(Connection&&) = default;
  ~Connection() { if (off_) off_(); }
};
int main(){
  { Connection a([]{ ++offs; }), b([]{ ++offs; });
    b = std::move(a);          // old b's subscription must be released here
  }
  assert(offs==2);             // each subscription unsubscribed exactly once
}
