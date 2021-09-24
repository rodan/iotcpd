#ifndef __DAEMON_GLUE_H_
#define __DAEMON_GLUE_H_

// statuses a daemon can be in
#define     S_NULL          0x00
#define     S_AVAILABLE     0x01 
#define     S_BUSY          0x02
#define     S_SPAWNING      0x04
#define     S_STARTING      0x08

#ifdef DAEMON_LEVEL
    #define DAEMON_EXPORT
#else
    #define DAEMON_EXPORT extern 
#endif

typedef enum _PIPE_END {
    PIPE_END_READ = 0,
    PIPE_END_WRITE = 1
} PIPE_END;

typedef struct {
    int status;
    int producer_pipe_fd[2];
    int consumer_pipe_fd[2];
    pid_t daemon_pid;
    pid_t client_pid;
    int client_fd;
    struct timespec start;
} daemon_t;

DAEMON_EXPORT daemon_t *d;

void *spawn(void *param);
void frag(void *param);
void update_status(int *avail_array, int *count);

#endif
