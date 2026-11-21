#include <memory>
#include <cassert>
struct Child; struct Parent { std::shared_ptr<Child> c; };
struct Child { std::shared_ptr<Parent> p; };
int main(){
  std::weak_ptr<Child> w;
  { auto p = std::make_shared<Parent>(); p->c = std::make_shared<Child>(); p->c->p = p; w = p->c; }
  assert(!w.expired());   // cycle: regression-test pattern DETECTS leak
}
