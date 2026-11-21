#include <cstdio>
#include <memory>
struct FileCloser { void operator()(FILE* f) const noexcept { if (f) std::fclose(f); } };
using FilePtr = std::unique_ptr<FILE, FileCloser>;
bool readf(FILE*){ return false; }
bool f(const char* p){ FilePtr fp{std::fopen(p,"rb")}; if(!fp) return false; return readf(fp.get()); }
int main(){ f("/dev/null"); }
