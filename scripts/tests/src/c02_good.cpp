#include <vector>
int g(int n, bool ok){ std::vector<int> p(n); if(!ok) return -1; return 0; }
int main(){ return g(10,false)==-1?0:1; }
