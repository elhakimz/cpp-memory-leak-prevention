struct Config { int retries; bool verbose; };
struct Server { int port; Server() {} };
int main(){ Server s; return s.port; }
