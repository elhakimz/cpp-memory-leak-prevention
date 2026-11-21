#include <cstring>
const char* longer(){ return "this string is clearly longer than eight bytes"; }
int main(){ char buf[8]; std::strcpy(buf, longer()); return buf[0]; }
