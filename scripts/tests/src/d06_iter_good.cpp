#include <vector>
int main(){ std::vector<int> v{1,2,3}; std::vector<int> extra;
  for (auto& x : v) if (x == 1) extra.push_back(9);
  v.insert(v.end(), extra.begin(), extra.end()); return v.size() == 4 ? 0 : 1; }
