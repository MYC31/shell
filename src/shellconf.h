#ifndef SHELLCONF_H
#define SHELLCONF_H
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "syswrap.h"
#include "assert.h"
#endif



/* prompt for shell */
#define PROMPT              "Shell"

/* cionfiguration for command line */
#define MAX_CMDARG_NUM      128
#define CMD_DLIM            " "
#define PIPE_DLIM           "|"
#define MAXLINE             512

/* signal for background and foreground job*/
#define BACK_PROG           1
#define FORE_PROG           0
#define HAND_JOB            ":)"    /* what's this? :) */

/* type of hash value */
typedef long                hash_t;

/* hash value for internal call */
#define CD_HASH             5863276
#define HIS_HASH            229468404218647
#define EXIT_HASH           6385204799
#define MYTOP_HASH          210721293598

/* type value for internal call */
/* deprecated!                  */    
#define CD                  0
#define HISTORY             1
#define MYTOP               2
#define EXIT                3

/* value or operation for history */
#define INIT_CAPACITY       2   /* should be 10 */
#define newcapacity(c)      (c << 1)

/* pattern of I/O redirction */
#define REDIR_INPUT         0
#define REDIR_OUTPUT        1
#define REDIR_APPEND        2

/* read and write end of pipe */
#define PIPE_READEND        0
#define PIPE_WRITEEND       1

/* determine the type of I/O redirection */
#define IS_REDIR_SIG(s)     (s[0]=='>' || s[0]=='<' || (s[0]=='>' && s[1]=='>'))


/* struct for I/O redirection */
typedef struct {
    int dir;        /* REDIR_INPUT or REDIR_OUTPUT */
    int sig_idx;    /* index of redir signal in argv */
    int oldfd;
} redir_sig;


/* operation on redir_sig */
static inline hash_t        hash(char * str);


/* hash value for redirection pattern */    
#define OUTPUT_HASH         177635
#define INPUT_HASH          177633
#define APPEND_HASH         5862017


/* data structure for handling process collection */
typedef struct {
    int mode;
    pid_t pid;
} task_struct;

// #define VALID_PID(task) ((task)->pid > 0)

typedef struct {
    int tasknum;
    task_struct **tasks;
} task_queue;

#define VALID_PID(queue,i)  ((queue)->tasks[(i)]->pid > 0)
#define IS_FOREJOB(queue,i) ((queue)->tasks[(i)]->mode == FORE_PROG)


/*
 * compute hash value of str
 * @param str   input string
 * @retval  hash value of str
 */
static inline hash_t hash(char * str) 
{
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    // printf("%lu\n", hash);
    return hash;
}


static task_struct * task_init(int mode, pid_t pid) 
{
    task_struct * task = malloc(sizeof(task_struct));
    task->mode = mode; task->pid = pid;
    return task;
}


static task_queue * task_queue_init(int tasknum) 
{
    task_queue * queue = malloc(sizeof(task_queue));
    queue->tasknum = tasknum;
    queue->tasks = malloc(tasknum*sizeof(task_struct *));
    return queue;
}



inline static void collect_single_task(task_struct * task) 
{
    if (task == NULL) return;
    if (task->mode == FORE_PROG) {
        Waitpid(task->pid, NULL, 0);
    }
    free(task);
}



static void collect_all_task(task_queue * queue)
{
    if (queue == NULL) return ;
    int i;
    for (i = 0; i < queue->tasknum; i++) {
        Waitpid(queue->tasks[i]->pid, NULL, 0);
    }
}


/* struct for handling closure of pipe in child process */
/* I start to realize that i might have made this thing complicate :< */
typedef struct {
    int (*pipes)[2];
    int pipenum;
} pipe_queue;


static pipe_queue * pipe_queue_init(int pipes[][2], int pipenum)
{
    pipe_queue * queue = malloc(sizeof(pipe_queue));
    assert(queue != NULL);
    queue->pipes = pipes; 
    queue->pipenum = pipenum;
    return queue;
}

typedef struct {
    pipe_queue * queue;
    int procnum;
} proc_pipe_info;


static proc_pipe_info * proc_pipe_info_init(int pipes[][2], int pipenum, int procnum)
{
    proc_pipe_info * info = malloc(sizeof(proc_pipe_info));
    assert(info != NULL);
    info->queue = pipe_queue_init(pipes, pipenum);
    info->procnum = procnum;
    return info;
}
