#include <vector>
struct Shape{ virtual ~Shape()=default; }; struct Circle:Shape{};
int main(){ std::vector<Shape*> s; s.push_back(new Circle); }
