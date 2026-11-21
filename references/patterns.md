# Leak Patterns: Before / After

## Contents
1. Exception between new and delete
2. Early return
3. shared_ptr cycle
4. C API handles
5. Owning raw pointer containers
6. Callbacks capturing shared_ptr
7. Raw owning members / Rule of Five
8. Missing virtual destructor
9. Factory returning raw pointer
10. Subscription never removed

## 1. Exception between new and delete
```cpp
// BAD
void f() { auto* b = new Buffer(1024); parse(*b); delete b; }   // parse may throw
// GOOD
void f() { auto b = std::make_unique<Buffer>(1024); parse(*b); }
```

## 2. Early return
```cpp
// BAD
int* p = new int[n]; if (!ok) return -1; /* leak */ delete[] p;
// GOOD
std::vector<int> p(n); if (!ok) return -1;
```

## 3. shared_ptr cycle
```cpp
// BAD: Parent <-> Child both shared_ptr, never freed
struct Child; struct Parent { std::shared_ptr<Child> c; };
struct Child { std::shared_ptr<Parent> p; };
// GOOD
struct Child { std::weak_ptr<Parent> p; };   // lock() before use
```

## 4. C API handles
```cpp
// BAD (note: LeakSanitizer will NOT report this; check fds with valgrind --track-fds=yes / lsof)
FILE* f = fopen(p, "r"); if (!read(f)) return false; fclose(f);
// GOOD
struct FileCloser { void operator()(FILE* f) const noexcept { if (f) std::fclose(f); } };
using FilePtr = std::unique_ptr<FILE, FileCloser>;
FilePtr f{std::fopen(p, "r")}; if (!f) return false; if (!read(f.get())) return false;
```
Avoid `std::unique_ptr<FILE, decltype(&fclose)>`: on GCC it triggers `-Wignored-attributes` (an error under `-Wall -Werror`).
Generic handle wrapper:
```cpp
template <class H, void(*Close)(H)>
class Handle {
  H h_{}; bool valid_ = false;
public:
  Handle() = default;
  explicit Handle(H h) : h_(h), valid_(true) {}
  Handle(Handle&& o) noexcept : h_(o.h_), valid_(std::exchange(o.valid_, false)) {}
  Handle& operator=(Handle&& o) noexcept { if (this != &o) { reset(); h_ = o.h_; valid_ = std::exchange(o.valid_, false);} return *this; }
  Handle(const Handle&) = delete; Handle& operator=(const Handle&) = delete;
  ~Handle() { reset(); }
  void reset() { if (valid_) { Close(h_); valid_ = false; } }
  H get() const { return h_; }
};
```

## 5. Owning raw pointer containers
```cpp
// BAD
std::vector<Shape*> shapes; shapes.push_back(new Circle);   // must delete each
// GOOD
std::vector<std::unique_ptr<Shape>> shapes; shapes.push_back(std::make_unique<Circle>());
```

## 6. Callbacks capturing shared_ptr
```cpp
// BAD: timer_ stored in this, lambda holds shared_ptr to this -> cycle
timer_.onTick([self = shared_from_this()] { self->tick(); });
// GOOD
timer_.onTick([weak = weak_from_this()] { if (auto s = weak.lock()) s->tick(); });
```

## 7. Raw owning members / Rule of Five
```cpp
// BAD: implicit copy double-frees or shallow-copies
class Img { uint8_t* d_; public: Img(size_t n):d_(new uint8_t[n]){} ~Img(){delete[] d_;} };
// GOOD (Rule of Zero)
class Img { std::vector<uint8_t> d_; public: explicit Img(size_t n):d_(n){} };
```

## 8. Missing virtual destructor
```cpp
struct Base { virtual ~Base() = default; virtual void run() = 0; };
```

## 9. Factory returning raw pointer
```cpp
// BAD
Widget* make();
// GOOD
std::unique_ptr<Widget> make();
```

## 10. Subscription never removed (logical leak)
```cpp
class Connection {                       // RAII unsubscribe token
  std::function<void()> off_;
public:
  Connection() = default;
  explicit Connection(std::function<void()> off) : off_(std::move(off)) {}
  Connection(Connection&& o) noexcept : off_(std::exchange(o.off_, nullptr)) {}
  Connection& operator=(Connection&& o) noexcept {
    if (this != &o) { reset(); off_ = std::exchange(o.off_, nullptr); }
    return *this;
  }
  Connection(const Connection&) = delete; Connection& operator=(const Connection&) = delete;
  ~Connection() { reset(); }
  void reset() noexcept { if (off_) { auto f = std::exchange(off_, nullptr); f(); } }
};
// subscribe() returns Connection; store it as a member so it dies with the subscriber.
```
Pitfall (verified): `Connection& operator=(Connection&&) = default;` silently leaks the *old* subscription on assignment, and a moved-from `std::function` is only "valid but unspecified", so always empty it explicitly with `std::exchange`.
