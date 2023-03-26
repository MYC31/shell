#include "stdio.h"
#include "unistd.h"
#include "errno.h"
#include "string.h"
#include "stdlib.h"


#define BUFFSIZE 1024


static void transferinfo() 
{
    static char buff[BUFFSIZE];
    if (read(STDIN_FILENO, buff, BUFFSIZE) < 0) {
        fprintf(stderr, "[%d] %s", getpid(), strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("%s", buff);
    exit(EXIT_SUCCESS);
} 



int main(int argc, char *argv[]) 
{
    transferinfo();
    return 0;
}