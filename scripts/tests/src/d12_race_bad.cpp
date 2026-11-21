#include <thread>
int counter = 0;
int main(){ std::thread a([]{ for (int i=0;i<1000;++i) ++counter; }); std::thread b([]{ for (int i=0;i<1000;++i) ++counter; }); a.join(); b.join(); }
