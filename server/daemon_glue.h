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

void *spawn(void *param);
int producer_process(void *param);
int consumer_process(void *param);


#endif
