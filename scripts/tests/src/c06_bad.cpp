#include <memory>
#include <functional>
struct Timer { std::function<void()> cb; void onTick(std::function<void()> f){ cb=std::move(f);} };
struct W : std::enable_shared_from_this<W> {
  Timer timer_; void tick(){}
  void start(){ timer_.onTick([self = shared_from_this()]{ self->tick(); }); } };
int main(){ auto w = std::make_shared<W>(); w->start(); }
