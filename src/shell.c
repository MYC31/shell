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
static int cmdparse(char *cmd, char **argv, const char *dlim) 
{
    char *token = strtok(cmd, dlim);
    int argc = 0;
    while (token != NULL) {
        argv[argc] = malloc(strlen(token));
        strcpy(argv[argc], token);
        token = strtok(NULL, dlim);
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
    // if (hashval == CD_HASH || hashval == HIS_HASH 
    //     || hashval == EXIT_HASH || hashval == MYTOP_HASH) {
    //         rval = 1;
    //     } else { rval = 0; }
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

        // if (hashval == INPUT_HASH) {
        //     sig->dir = REDIR_INPUT;
        //     sig->oldfd = dup(STDIN_FILENO);
        //     int newfd = open(argv[i+1], O_RDONLY, S_IRUSR);
        //     dup2(newfd, STDIN_FILENO);
        // } else if (hashval == OUTPUT_HASH) {
        //     sig->dir = REDIR_OUTPUT;
        //     sig->oldfd = dup(STDOUT_FILENO);
        //     int newfd = open(argv[i+1], O_WRONLY|O_CREAT, S_IWUSR);
        //     dup2(newfd, STDOUT_FILENO);
        // } else if (hashval == APPEND_HASH) {
        //     sig->dir = REDIR_APPEND;
        //     sig->oldfd = dup(STDOUT_FILENO);
        //     int newfd = open(argv[i+1], O_APPEND|O_CREAT|O_WRONLY, S_IWUSR);
        //     dup2(newfd, STDOUT_FILENO);
        // } else {
        //     fprintf(stderr,"invalid parameter for I/O redirection!\n");
        //     fflush(stderr);
        // }
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
        // if (sig->dir == REDIR_INPUT) {
        //     dup2(sig->oldfd, STDIN_FILENO);
        //     close(sig->oldfd);
        // } else if (sig->dir == REDIR_OUTPUT ||
        //             sig->dir == REDIR_APPEND) {
        //     dup2(sig->oldfd, STDOUT_FILENO);
        //     close(sig->oldfd);
        // }
        free(sig);
    }
}



/* not considering the collision between pipe and IO redirection */
static task_struct * run_single_process(char *cmdline, char **envp, proc_pipe_info * info) /* cmdline is a copy of original command line */
{
    /* copy cmdline */
    char cmdline_cpy[MAXLINE];
    strcpy(cmdline_cpy, cmdline);
    char *buff[MAX_CMDARG_NUM];

    /* parse the command line and put the result into buff */
    int argnum = cmdparse(cmdline, (char **)buff, CMD_DLIM);
    int jobtype = isinternal(buff, argnum);
    int mode = isbackground(cmdline_cpy);

    /* delete the last '&' in cmd of background job */
    if (mode == BACK_PROG) {
        free(buff[argnum-1]);
        buff[argnum-1] = NULL;
        argnum--;
    }

    redir_sig * sig = setredir(argnum, buff);
    /* change the parsed cmd when I/O redirection is implemented */
    /* delete the redundent arg in buff */
    if (sig != NULL) {
        for (int i=sig->sig_idx; i < argnum; i++) {
            free(buff[i]); 
            buff[i] = NULL;
        }
        argnum = sig->sig_idx+1;
    }

    /* execute the job */
    pid_t pid;
    task_struct * task;
    if (jobtype == 1) { /* internal case */
        /* let dointernal support info parameter later... */
        pid = dointernal(hash(buff[0]), argnum, buff, mode);
        task = NULL;
    } else {
        pid = doprogram(argnum, buff, envp, mode, info);
        task = task_init(mode, pid);
    }

    /* unset potential I/O redirection */
    unsetredir(sig);

    /* free buff for next job */
    for (int i=0; i < argnum; i++) {
        free(buff[i]);
    }

    return task;
}






/* pipe process is identified as background job */
/* beaware to close all the duplicate pipes in process group, because as lon as a fd is pointed to a file/pipe, 
    the OS will keep that file open and the process that read from that fd will not read an EOF, thus keeping running and 
    refusing to be collected. */
/* consider stripping run_pipe_process from run_single_process */
static task_queue * run_pipe_process(int argc, char **buff, char **envp)
{
    task_queue * queue = task_queue_init(argc);

    /* initialize pipe array */
    // int pipe[argc-1][2];
    int pipenum = argc-1;
    // int **pipes = malloc(2*pipenum*sizeof(int));
    int pipes[pipenum][2];
    for (int i=0; i < pipenum; i++)
        Pipe(pipes[i]);

    /* save stdin and stdout */
    int stdin_fd  = dup(STDIN_FILENO);
    int stdout_fd = dup(STDOUT_FILENO);

    proc_pipe_info *info;
    /* initialize the first pipe process */
    dup2(pipes[0][PIPE_WRITEEND], STDOUT_FILENO);
    info = proc_pipe_info_init(pipes, pipenum, 0);
    queue->tasks[0] = run_single_process(buff[0], envp, info);

    int pid_idx;
    /* proc_i   pipe_i   proc_i+1 */
    for (pid_idx=1; pid_idx < argc-1; pid_idx++) {
        dup2(pipes[pid_idx-1][PIPE_READEND], STDIN_FILENO);
        dup2(pipes[pid_idx][PIPE_WRITEEND], STDOUT_FILENO);
        info = proc_pipe_info_init(pipes, pipenum, pid_idx);
        queue->tasks[pid_idx] = run_single_process(buff[pid_idx], envp, info);
    }

    /* initialize the last pipe process*/
    dup2(pipes[pipenum-1][PIPE_READEND], STDIN_FILENO);
    dup2(stdout_fd, STDOUT_FILENO);
    info = proc_pipe_info_init(pipes, pipenum, argc-1);
    queue->tasks[argc-1] = run_single_process(buff[argc-1], envp, info);

    /* all pipe processed exit, close pipe */
    /* be aware to put this piece of code before free_task_queue!!! */
    for (int i=0; i < pipenum; i++) {
        close(pipes[i][PIPE_READEND]);
        close(pipes[i][PIPE_WRITEEND]);
    }

    /* retrieve stdin file descriptor */
    dup2(stdin_fd, STDIN_FILENO);

    /* close the saved stdin and stdout file descriptor */
    close(stdin_fd);
    close(stdout_fd);

    return queue;
}


/* stucked in the loop, cannot collect the "grep" process!!! */
/* code is right, but program "grep" somehow fails to exit */
static void wait_task_queue(task_queue * queue)
{
    if (queue == NULL) return ;
    int i;
    for (i = 0; i < queue->tasknum; i++) {
        if (queue->tasks[i] == NULL) continue;
        if (VALID_PID(queue,i) && IS_FOREJOB(queue,i)) {
            pid_t pid = Waitpid(queue->tasks[i]->pid, NULL, 0);
            /* test code */
            fprintf(stdout, "[%d]th: P[%d] has been collected\n", i, pid);
        }
        fflush(stdout);
        free(queue->tasks[i]);
    }
    free(queue);
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

        /* copy the command line */
        char cmdline_cpy[MAXLINE];
        strcpy(cmdline_cpy, cmdline);

        /* initial parse of command line needed here */
        char *buff[MAX_CMDARG_NUM];

        /* parse the potential pipe process */
        int process_num = cmdparse(cmdline_cpy, buff, PIPE_DLIM);

        /* logic error, should fork for every process in pipe */
        if (process_num <= 0) {
            fprintf(stdout, "command parse failed!\n");
            fflush(stdout);
        } else if (process_num == 1) {
            task_struct * task = run_single_process(buff[0], envp, NULL);
            collect_single_task(task);
        } else { /* more than 2 processes and pipe needed */
            task_queue * queue = run_pipe_process(process_num, buff, envp);
            wait_task_queue(queue);
        }
        
    }

    exit(EXIT_SUCCESS);
}



