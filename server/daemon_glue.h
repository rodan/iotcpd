#ifndef __DAEMON_GLUE_H_
#define __DAEMON_GLUE_H_

// statuses a daemon can be in
#define     S_NULL          0
#define     S_AVAILABLE     0x1 
#define     S_BUSY          0x2
#define     S_DEAD          0x4

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

daemon_t *d;

void *spawn(void *param);
void update_status(int *avail_array, int *count);

#endif
