#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"


int main(int argc, char *argv[])
{
    #define PRONUM 10
    for (int i = 0; i < PRONUM; i++)
    {
        pid_t pid = fork();
        if (pid == 0) {
            execve("test", NULL, NULL);
        } else {
            wait(NULL);
        }
    }
    exit(EXIT_SUCCESS);
}