#ifndef HISTORY_H
#define HISTORY_H

#include "stdio.h"


char **init_history();
char **addhistory(char **history, const char *cmd);


#endif