#include <cstdio>
bool readf(FILE*){ return false; }
bool f(const char* p){ FILE* f = fopen(p,"r"); if(!readf(f)) return false; fclose(f); return true; }
int main(){ f("/dev/null"); }
