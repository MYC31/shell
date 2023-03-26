#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "shellconf.h"
#include "shelljob.h"
#include "unistd.h"
#include "fcntl.h"
#include "syswrap.h"


/* global pointer to history */
char **         history;
/* crusor of history */
size_t          crusor;
/* capacity of history */
static size_t   capacity;




/*
 * parse the cmd and store args in argv
 * @param cmd   command line input
 * @param argv  buffer to store parsed argv
 * @retval number of args
*/
static int cmdparse(char *cmd, char **argv) 
{
    char *token = strtok(cmd, CMD_DLIM);
    int argc = 0;
    while (token != NULL) {
        argv[argc] = malloc(strlen(token));
        strcpy(argv[argc], token);
        token = strtok(NULL, CMD_DLIM);
        argc++;
    }
    /* added code */
    argv[argc] = NULL;

    return argc;
}  



/*
 * judge if the cmd is background job
 * @param cmd   command line input
 * @retval  1 if command is background, 0 otherwise
 */
static int isbackground(const char *cmd) 
{
    size_t len = strlen(cmd);
    return (cmd[len-1]=='&') ? 
            BACK_PROG : FORE_PROG;
}



/*
 * judge if the command is internal job
 * @param argv  parsed command line input
 * @param argc  number of string in argv
 * @retval  1 if process is internal, 0 otherwise
 */
static int isinternal(char ** argv, int argc)
{
    int rval;
    hash_t hashval = hash(argv[0]);
    switch (hashval) {
        case CD_HASH: 
        case HIS_HASH:
        case EXIT_HASH: 
        case MYTOP_HASH:    rval = 1;   break;
        default:            rval = 0;
    }
    return rval;
}



/*
 * initialize a history array 
 * @retval  initialized history.
 */
static char **init_history()
{
    crusor = 0;
    capacity = INIT_CAPACITY;
    history = malloc(INIT_CAPACITY*sizeof(char *));
    assert(history != NULL);
    for (size_t i = 0; i < INIT_CAPACITY; i++) {
        history[i] = malloc(MAXLINE*sizeof(char));
    }
    return history;
}



/*
 * Add the command line into the history.
 * @param history   store the command line since the shell started
 * @param cmd       the new command line that needs storage
 * @retval          Pointer to history. 
 *                  NULL if failure, history if success.
 */
static char **addhistory(char **history, const char *cmd)
{
    /* history needs expansion */
    if (crusor == capacity) {
        /* expand the history */
        size_t new_capacity = newcapacity(capacity);
        history = realloc(history, new_capacity*sizeof(char *));
        assert(history != NULL);
        for (size_t i=capacity; i < new_capacity; i++)
            history[i] = malloc(MAXLINE*sizeof(char));
        capacity = new_capacity;
    }

    strcpy(history[crusor], cmd);
    crusor++;

    return history;
}



/*
 * check if the command line contains I/O redirection symbols
 * @param argc      number of args in argv
 * @param argv      parsed command line input
 * @retval          1 with I/O redirection, 0 otherwise
 */
static int needredir(int argc, char **argv) 
{
    int i;
    for (i=0; i < argc; i++)
        if (IS_REDIR_SIG(argv[i])) break;
    return (i==argc) ? -1 : i;
}



/*
 * correctly set I/O redirection for the programme
 * @param argc      number of args in argv
 * @param argv      parsed command line input
 * @retval          a struct redir_sig including info about the redirection   
 */
static redir_sig * setredir(int argc, char **argv)
{
    redir_sig *sig = NULL;
    int i = needredir(argc, argv);

    if (i >= 0) 
    {
        sig = malloc(sizeof(redir_sig));
        assert(sig != NULL);
        sig->sig_idx = i;
        hash_t hashval = hash(argv[i]);
        // printf("redir:%s hashval:%lu\n", argv[i], hashval);
        switch (hashval) {
            case INPUT_HASH:
            {
                sig->dir = REDIR_INPUT;
                sig->oldfd = dup(STDIN_FILENO);
                int newfd = open(argv[i+1], O_RDONLY, S_IRUSR);
                dup2(newfd, STDIN_FILENO);
                break;
            }
            case OUTPUT_HASH:
            {
                sig->dir = REDIR_OUTPUT;
                sig->oldfd = dup(STDOUT_FILENO);
                int newfd = open(argv[i+1], O_WRONLY|O_CREAT, S_IWUSR);
                dup2(newfd, STDOUT_FILENO);
                break;
            }
            case APPEND_HASH: 
            {
                sig->dir = REDIR_APPEND;
                sig->oldfd = dup(STDOUT_FILENO);
                int newfd = open(argv[i+1], O_APPEND|O_CREAT|O_WRONLY, S_IWUSR);
                dup2(newfd, STDOUT_FILENO);
                break;
            }
            default:
            {
                fprintf(stderr,"invalid parameter for I/O redirection!\n");
                fflush(stderr);
            }
        }
    }
    return sig;
}



/* 
 * correctly unset I/O redirection for the programme
 * @param sig       a struct redir_sig including info about the redirection        
 */
static void unsetredir(redir_sig *sig)
{
    if (sig != NULL) {
        switch (sig->dir) 
        {
            case REDIR_INPUT:
                dup2(sig->oldfd, STDIN_FILENO);
                close(sig->oldfd);
                break;
            case REDIR_OUTPUT: 
            case REDIR_APPEND: 
                dup2(sig->oldfd, STDOUT_FILENO);
                close(sig->oldfd);
                break;
        }
        free(sig);
    }
}




static void run_single_process(char *cmdline, char **envp) /* cmdline is a copy of original command line */
{
    /* copy cmdline */
    char cmdline_cpy[MAXLINE];
    strcpy(cmdline_cpy, cmdline);

    /* set command buff */
    char *buff[MAX_CMDARG_NUM];

    /* parse the command line and put the result into buff */
    int argnum = cmdparse(cmdline, (char **)buff);


    /* get jobtype and mode of given job */
    int jobtype = isinternal(buff, argnum);
    int mode = isbackground(cmdline_cpy);

    /* delete the last '&' in cmd of background job */
    if (mode == BACK_PROG) {
        free(buff[argnum-1]);
        buff[argnum-1] = NULL;
    }

    /* set potential I/O redirection */
    redir_sig * sig = setredir(argnum, buff);
    
    /* change the parsed cmd when I/O redirection is implemented */
    if (sig != NULL) {
        for (int i=sig->sig_idx; i < argnum; i++) {
            free(buff[i]); 
            buff[i] = NULL;
        }
        argnum = sig->sig_idx+1;
    }

    /* execute the job */
    pid_t pid;
    if (jobtype == 1) { /* internal case */
        pid = dointernal(hash(buff[0]), argnum, buff, mode);
    } else {
        pid = doprogram(argnum, buff, envp, mode);
    }

    /* possible collection of the subroutine */
    if (pid > 0 && mode == FORE_PROG) {
        waitpid(pid, NULL, 0);
    }

    /* unset potential I/O redirection */
    unsetredir(sig);


    /* free buff for next job */
    for (int i=0; i < argnum; i++) {
        // printf("%s", buff[i]);
        // if (i != argnum-1) printf(" | ");
        free(buff[i]);
    }
    // printf("\n-----------\n\n");

}






int main(int argc, char **argv, char **envp)
{
    /* command line buffer */
    char cmdline[MAXLINE];

    /* initialize buffer for history job */
    history = init_history();

    /* working directory used for shell prompt */
    char workdir[MAXLINE];


    while (1) {
        getcwd(workdir, MAXLINE);
        fprintf(stdout, "%s > ", workdir); fflush(stdout);
        fgets(cmdline, MAXLINE, stdin);

        /* update the history, with the last '\n' unremoved */
        history = addhistory(history, cmdline);


        /* remove the last '\n' of cmdline */
        size_t len = strlen(cmdline);
        cmdline[len-1] = '\0';


        char cmdline_cpy[MAXLINE];
        strcpy(cmdline_cpy, cmdline);





        /* initial parse of command line needed here */



        /** ------factorization starts here------ **/
        /* param: cmdline_cpy, envp*/

        //for loop needed here for processes in pipe

        run_single_process(cmdline_cpy, envp);

        /* ------factorization ends here------ */



    }

    exit(EXIT_SUCCESS);
}



