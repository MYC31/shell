#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"


int main(int argc, char *argv[])
{
    pid_t pid = getpid();
    printf("pid = %d\n", pid);
    exit(EXIT_SUCCESS);
}