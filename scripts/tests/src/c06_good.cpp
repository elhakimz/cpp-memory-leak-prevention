#include <memory>
#include <functional>
struct Timer { std::function<void()> cb; void onTick(std::function<void()> f){ cb=std::move(f);} };
struct W : std::enable_shared_from_this<W> {
  Timer timer_; void tick(){}
  void start(){ timer_.onTick([weak = weak_from_this()]{ if(auto s = weak.lock()) s->tick(); }); } };
int main(){ auto w = std::make_shared<W>(); w->start(); w->timer_.cb(); }
