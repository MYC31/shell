#include "stdio.h"
#include "stdlib.h"
#include "string.h"




int main(int argc, char *argv[])
{
    for (int i=2; i < argc; i++) {
        if (strstr(argv[i], argv[1]) != NULL) 
            printf("%s\n", argv[i]);
    }
    return 0;
}



