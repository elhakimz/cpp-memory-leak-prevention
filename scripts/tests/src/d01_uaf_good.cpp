#include <memory>
int main(){ auto p = std::make_unique<int>(5); int v = *p; p.reset(); return v == 5 ? 0 : 1; }
