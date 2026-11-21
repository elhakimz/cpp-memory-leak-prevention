#include <string>
const char* longer(){ return "this string is clearly longer than eight bytes"; }
int main(){ std::string s = longer(); return s[0] == 't' ? 0 : 1; }
