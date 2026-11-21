#include <utility>
#include <cassert>
static int opened=0, closed=0;
void closeInt(int){ ++closed; }
template <class H, void(*Close)(H)>
class Handle {
  H h_{}; bool valid_ = false;
public:
  Handle() = default;
  explicit Handle(H h) : h_(h), valid_(true) {}
  Handle(Handle&& o) noexcept : h_(o.h_), valid_(std::exchange(o.valid_, false)) {}
  Handle& operator=(Handle&& o) noexcept { if (this != &o) { reset(); h_ = o.h_; valid_ = std::exchange(o.valid_, false);} return *this; }
  Handle(const Handle&) = delete; Handle& operator=(const Handle&) = delete;
  ~Handle() { reset(); }
  void reset() { if (valid_) { Close(h_); valid_ = false; } }
  H get() const { return h_; }
};
using IH = Handle<int, closeInt>;
int main(){
  { IH a(1), b(2); ++opened; ++opened;
    IH c(std::move(a));       // a empty
    b = std::move(c);         // closes old b(2); b now owns 1
    IH d; IH& dr = d; d = std::move(dr);   // self-move safe
  }
  assert(closed==2);
}
