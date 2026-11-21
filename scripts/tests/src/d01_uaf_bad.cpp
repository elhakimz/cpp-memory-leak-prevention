int main(){ int* p = new int(5); delete p; return *p == 5 ? 0 : 1; }
