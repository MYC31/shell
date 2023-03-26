#ifndef SHELLJOB_H 
#define SHELLJOB_H
#include "sys/wait.h"
#endif

#ifndef SHELLCONF_H
#define SHELLCONF_H
#include "shellconf.h"
#endif


pid_t doprogram(int argc, char **argv, char **envp, int mode, proc_pipe_info * info);
pid_t dointernal(long type, int argc, char **argv, int mode);
