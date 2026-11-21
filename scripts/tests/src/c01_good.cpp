#include <memory>
#include <stdexcept>
struct Buffer { int* d; Buffer(int n):d(new int[n]){} ~Buffer(){delete[] d;} };
void parse(Buffer&){ throw std::runtime_error("x"); }
void f(){ auto b = std::make_unique<Buffer>(1024); parse(*b); }
int main(){ try{ f(); }catch(...){} }
