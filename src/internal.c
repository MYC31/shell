/*
 * implementation of dointernal
 */
#include "sys/wait.h"
#include "shellconf.h"
#include "syswrap.h"
#include "stdlib.h"
#include "assert.h"
#include "fcntl.h"



/* pointer to history in shell.c */
extern char **      history;
/* crusor of history in shell.c */
extern size_t       crusor;


#define MIN(n,m) ((n < m) ? n : m)

#define INORDER_HISTORY         0
#define REVERSE_HISTORY         1

#define HISTORY_MODE(n) ((n)==crusor ? INORDER_HISTORY : REVERSE_HISTORY)



static void cd(int argc, char **argv);
static void phistory(int argc, char **argv);
static void mytop(int argc, char **argv);
static void myExit(int argc, char **argv);




/*
 * core function that implements internal command
 * @type            specify the type among four ones
 * @param argc      number of arg in argv
 * @param argv      string array of parsed command line
 * @retval          possible pid of background processs   
 */
pid_t dointernal(long type, int argc, char **argv, int mode)
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
    // if (type == CD_HASH) {
    //     func = cd;
    // } else if (type == HIS_HASH) {
    //     func = phistory;
    // } else if (type == EXIT_HASH) {
    //     func = myExit;
    // } else if (type == MYTOP_HASH) {
    //     func = mytop;
    // } else {
    //     func = NULL;
    // }

    assert(func != NULL);

    if (mode == FORE_PROG) {
        func(argc, argv);
        pid = -1;
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



inline static void print_single_history(const int index, int mode)
{
    if (mode == INORDER_HISTORY) {
        printf("%s", history[index]);
    } else {
        printf("%s", history[crusor-index]);
    }
}


/*
    bug in history
*/
static void phistory(int argc, char **argv)
{
    assert(history !=  NULL);
    int n = (argc > 1) ? atoi(argv[1]):crusor;
    if (n < 0) {
        printf("invalid argument for command 'history'\n");
        return;
    }
    int mode = HISTORY_MODE(n);
    int index;
    for (index=0; index < n; index++) {
        print_single_history(index, mode);
    }
}



static void mytop(int argc, char **argv)
{
    // if (Execve("/usr/bin/top", NULL, NULL) < 0)
    //     exit(EXIT_FAILURE);
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
    // fprintf(stdout, "exiting Shell...\n");
    // fflush(stdout);
    exit(EXIT_SUCCESS);
}





