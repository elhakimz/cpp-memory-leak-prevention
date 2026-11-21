#include <stdexcept>
struct Buffer { int* d; Buffer(int n):d(new int[n]){} ~Buffer(){delete[] d;} };
void parse(Buffer&){ throw std::runtime_error("x"); }
void f(){ auto* b = new Buffer(1024); parse(*b); delete b; }
int main(){ try{ f(); }catch(...){} }
