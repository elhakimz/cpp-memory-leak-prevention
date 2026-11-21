#include <functional>
std::function<int()> make(){ int local = 42; return [local]{ return local; }; }
int main(){ auto f = make(); return f() == 42 ? 0 : 1; }
