#ifndef __MAIN_H_
#define __MAIN_H_

#include <inttypes.h>

#define MAX_DAEMONS 32
#define MSG_MAX 4096

///////////////////////////////
// default values that can be changed via 
// command line arguments
//

int debug;
int num_daemons;
int port;
char *ip4;
char *ip6;
char *daemon_str;
char **daemon_array;

///////////////////////////////

volatile int all_busy;

typedef struct {
    int status;
    int producer_pipe_fd[2];
    int consumer_pipe_fd[2];
    pid_t daemon_pid;
    pid_t client_pid;
    int client_fd;
    time_t time;
} daemon_t;

daemon_t *d;

typedef struct {
    int spawns;
    int daemon_deaths;
    int daemon_respawns;
    int total_queries;
    int replied_queries;
    int delayed_queries;
    int failed_queries;
    int d_avail;
    int d_busy;
    int d_dead;
} status_t;

status_t st;

void signal_handler(const int signo);
void update_status(int *avail_array, int *count);

#endif
