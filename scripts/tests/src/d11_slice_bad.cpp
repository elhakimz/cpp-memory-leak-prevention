#include <vector>
#include <cassert>
struct Base { virtual ~Base() = default; virtual int id() const { return 1; } };
struct Derived : Base { int id() const override { return 2; } };
int main(){ std::vector<Base> v; v.push_back(Derived{}); assert(v[0].id() == 2); }
