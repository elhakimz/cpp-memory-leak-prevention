#include <vector>
#include <memory>
struct Shape{ virtual ~Shape()=default; }; struct Circle:Shape{};
int main(){ std::vector<std::unique_ptr<Shape>> s; s.push_back(std::make_unique<Circle>()); }
