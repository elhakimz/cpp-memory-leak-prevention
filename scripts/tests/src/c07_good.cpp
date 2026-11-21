#include <cstdint>
#include <vector>
class Img { std::vector<uint8_t> d_; public: explicit Img(size_t n):d_(n){} };
int main(){ Img a(8); Img b = a; }
