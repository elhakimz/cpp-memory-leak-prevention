#include <string>
int main(){ std::string keep; { std::string s(100, 'x'); keep = s; } return keep[0] == 'x' ? 0 : 1; }
