#include <memory>
struct Child; struct Parent { std::shared_ptr<Child> c; };
struct Child { std::shared_ptr<Parent> p; };
int main(){ auto p = std::make_shared<Parent>(); p->c = std::make_shared<Child>(); p->c->p = p; p.reset(); }
