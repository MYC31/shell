#include "stdio.h"
#include "unistd.h"
#include "errno.h"
#include "string.h"
#include "stdlib.h"


#define BUFFSIZE 1024


/* collaborate with ls command */


int main()
{
    char buff[BUFFSIZE];
    if (read(STDIN_FILENO, buff, BUFFSIZE) < 0) {
        fprintf(stdout, "error in piperead\n"); fflush(stdout);
        fprintf(stderr, "[%d] %s\n", getpid(), strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("%s", buff);
    exit(EXIT_SUCCESS);
}