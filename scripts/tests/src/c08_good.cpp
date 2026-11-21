#include <vector>
struct Base { virtual ~Base() = default; virtual void run(){} };
struct Derived : Base { std::vector<int> v = std::vector<int>(100); };
int main(){ Base* p = new Derived; delete p; }
