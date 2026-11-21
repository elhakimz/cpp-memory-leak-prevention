struct Server { int port{8080}; Server() = default; };
int main(){ Server s; return s.port == 8080 ? 0 : 1; }
