#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
int main(){ int fd = open("/dev/null", O_RDONLY); (void)fd; /* never closed */ }
