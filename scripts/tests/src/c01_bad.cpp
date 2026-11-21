#include <stdexcept>
#include <vector>
struct Buffer { std::vector<int> d; explicit Buffer(int n) : d(n) {} };
void parse(Buffer&){ throw std::runtime_error("x"); }
void f(){ auto* b = new Buffer(1024); parse(*b); delete b; }
int main(){ try{ f(); }catch(...){} }
