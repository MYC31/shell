/*
 * implementation of dointernal
 */
#include "sys/wait.h"
#include "shellconf.h"
#include "syswrap.h"
#include "stdlib.h"
#include "assert.h"
#include "fcntl.h"
#include "pipehandler.h"
#include "history.h"



/* pointer to history in shell.c */
extern char **      history;
/* crusor of history in shell.c */
extern size_t       crusor;


#define MIN(n,m) ((n < m) ? n : m)

#define INORDER_HISTORY         0
#define REVERSE_HISTORY         1

#define HISTORY_MODE(n)         ((n)==crusor ? INORDER_HISTORY : REVERSE_HISTORY)
#define HISTORY_START(n)        (crusor-(n))



static void cd(int argc, char **argv);
static void phistory(int argc, char **argv);
static void mytop(int argc, char **argv);
static void myExit(int argc, char **argv);




/* param: info->queue->pipes, info->queue->pipenum, info->procnum */
static void handle_proc_pipe_info(proc_pipe_info * info);




/*
 * core function that implements internal command
 * @type            specify the type among four ones
 * @param argc      number of arg in argv
 * @param argv      string array of parsed command line
 * @retval          possible pid of background processs   
 */
pid_t dointernal(long type, int argc, char **argv, int mode, proc_pipe_info * info)
{
    pid_t pid;
    void (*func)(int,char **);
    
    /* set the correct function pointer */
    switch (type) {
        case CD_HASH:       func = cd;          break;
        case HIS_HASH:      func = phistory;    break;
        case EXIT_HASH:     func = myExit;      break;
        case MYTOP_HASH:    func = mytop;       break;
        default:            func = NULL;
    }
    assert(func != NULL);

    if (mode == FORE_PROG) {
        if (info == NULL) {
            func(argc, argv);
            pid = -1;
        } else {
            pid = Fork();
            if (pid == 0) {
                handle_proc_pipe_info(info);
                func(argc, argv);
                exit(EXIT_SUCCESS);
            }
            // free(info->queue);
            // free(info);
        }
    } else if (mode == BACK_PROG) {
        pid = Fork();
        if (pid == 0) {
            /* code for UNIX system */
            int fd = open("/dev/null", O_RDWR, S_IRWXU);
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            Signal(SIGCHLD, SIG_IGN);
            func(argc, argv);
            exit(EXIT_SUCCESS);
        } else {
            /* no collection needed for background process */
            pid = -1;
        }
    } else {
        fprintf(stdout, "error in dointernal!\n");
        fflush(stdout);
    }
    return pid;
}



/*
 * implementation of changing directory
 * @param argc      number of arg in argv
 * @param argv      string array of parsed command line
 */
static void cd(int argc, char **argv)
{
    Chdir(argv[1]);
    /* test code */
    static const int maxline = 100;
    char buff[maxline];
    getcwd(buff, maxline);
    fprintf(stdout,"current dir = %s\n",buff);
    fflush(stdout);
}




inline static int history_num(char *arg)
{
    if (arg == NULL) return crusor;
    int argval = atoi(arg);
    int rval;
    if (argval < 0) rval = 0;
    else if(argval >= 0 && argval <= crusor) rval = argval;
    else rval = crusor;
    return rval;
}



/*
 *   bug in history
 */
static void phistory(int argc, char **argv)
{
    assert(history !=  NULL);
    char *arg = (argc > 1) ? argv[1] : NULL;
    int num = history_num(arg);
    int mode = HISTORY_MODE(num);
    int index = HISTORY_START(num);
    for ( ; index < crusor; index++) {
        fprintf(stdout, "%s", history[index]);
    }
    fflush(stdout);
} 



static void mytop(int argc, char **argv)
{
    fprintf(stdout, "developing...\n");
    fflush(stdout);
}



/*
 * exit the shell
 * @param argc      number of arg in argv
 * @param argv      string array of parsed command line
 */
static void myExit(int argc, char **argv)
{
    exit(EXIT_SUCCESS);
}





