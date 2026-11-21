#include <optional>
std::optional<int> find(int){ return std::nullopt; }
int main(){ auto r = find(1); return r ? *r : 0; }
