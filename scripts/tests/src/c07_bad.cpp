#include <cstdint>
#include <cstddef>
class Img { uint8_t* d_; public: Img(size_t n):d_(new uint8_t[n]){} ~Img(){delete[] d_;} };
int main(){ Img a(8); Img b = a; }
