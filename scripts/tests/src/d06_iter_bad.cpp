#include <vector>
int main(){ std::vector<int> v{1,2,3}; for (auto& x : v) if (x == 1) v.push_back(9); return 0; }
