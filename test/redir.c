#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "errno.h"



#define BUFFSIZE (100*sizeof(char))
#define STRINGIZE(s) #s



int main(void)
{
    /* open error */
    int fd = open("file.txt", O_RDWR|O_CREAT, S_IWUSR);
    char *buff = malloc(BUFFSIZE*sizeof(char));
    strcpy(buff, "myc \n");

    /* dup stdout fd */
    int stdout_d = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);

    if (write(STDOUT_FILENO, buff, strlen(buff)) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
    }

    dup2(stdout_d, STDOUT_FILENO);
    printf("buff content: %s", buff);

    close(fd);
    close(stdout_d);

    exit(EXIT_SUCCESS);
}