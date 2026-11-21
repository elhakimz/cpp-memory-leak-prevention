#include <vector>
#include <stdexcept>
int main(int argc, char**){ std::vector<int> v(4);
  try { return v.at(argc + 3); } catch (const std::out_of_range&) { return 0; } }
