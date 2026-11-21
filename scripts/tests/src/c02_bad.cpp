int g(int n, bool ok){ int* p = new int[n]; if(!ok) return -1; delete[] p; return 0; }
int main(){ return g(10,false)==-1?0:1; }
