#include <string>
#include <string_view>
int main(){ std::string_view sv; { std::string s(100, 'x'); sv = s; } return sv[0] == 'x' ? 0 : 1; }
