#include <vector>
#include <memory>
#include <cassert>
struct Base { virtual ~Base() = default; virtual int id() const { return 1; } };
struct Derived : Base { int id() const override { return 2; } };
int main(){ std::vector<std::unique_ptr<Base>> v; v.push_back(std::make_unique<Derived>()); assert(v[0]->id() == 2); }
