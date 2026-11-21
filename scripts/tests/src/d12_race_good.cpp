#include <thread>
#include <atomic>
std::atomic<int> counter{0};
int main(){ std::thread a([]{ for (int i=0;i<1000;++i) ++counter; }); std::thread b([]{ for (int i=0;i<1000;++i) ++counter; }); a.join(); b.join(); return counter == 2000 ? 0 : 1; }
