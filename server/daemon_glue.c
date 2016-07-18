#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include "main.h"
#include "daemon_glue.h"

void *spawn(void *param)
{
    int os_rc = -1;
    daemon_t *d = (daemon_t *) param;

    // data producer => sg
    os_rc = pipe(d->producer_pipe_fd);
    if (os_rc == EXIT_FAILURE) {
        fprintf(stderr, "create pipe error '%s (%d)'\n", strerror(errno), errno);
        return NULL;
    }
    // data consumer <= sg
    os_rc = pipe(d->consumer_pipe_fd);
    if (os_rc == -1) {
        fprintf(stderr, "create pipe error '%s (%d)'\n", strerror(errno), errno);
        return NULL;
    }

    d->daemon_pid = fork();
    if (d->daemon_pid == -1) {
        fprintf(stderr, "fork sg error '%s (%d)'\n", strerror(errno), errno);
        return NULL;
    }
    if (d->daemon_pid == 0) {
        os_rc = dup2(d->producer_pipe_fd[PIPE_END_READ], STDIN_FILENO);
        if (os_rc == -1) {
            fprintf(stderr, "dup2 error '%s (%d)'\n", strerror(errno), errno);
            return NULL;
        }
        os_rc = close(d->producer_pipe_fd[PIPE_END_READ]);
        if (os_rc == -1) {
            fprintf(stderr, "close error '%s (%d)'\n", strerror(errno), errno);
            return NULL;
        }
        os_rc = close(d->producer_pipe_fd[PIPE_END_WRITE]);
        if (os_rc == -1) {
            fprintf(stderr, "close error '%s (%d)'\n", strerror(errno), errno);
            return NULL;
        }
        os_rc = close(d->consumer_pipe_fd[PIPE_END_READ]);
        if (os_rc == -1) {
            fprintf(stderr, "close error '%s (%d)'\n", strerror(errno), errno);
            return NULL;
        }
        os_rc = dup2(d->consumer_pipe_fd[PIPE_END_WRITE], STDOUT_FILENO);
        if (os_rc == -1) {
            fprintf(stderr, "dup2 error '%s (%d)'\n", strerror(errno), errno);
            return NULL;
        }
        os_rc = close(d->consumer_pipe_fd[PIPE_END_WRITE]);
        if (os_rc == -1) {
            fprintf(stderr, "close error '%s (%d)'\n", strerror(errno), errno);
            return NULL;
        }

        os_rc = execvp(daemon_array[0], daemon_array);
        if (os_rc == -1) {
            fprintf(stderr, "execvp error '%s (%d)'\n", strerror(errno), errno);
            return NULL;
        }
    }
    if (d->daemon_pid > 0) {
        d->status = S_STARTING;
        st.daemon_spawns++;
    }

    return NULL;
}

void frag(void *param)
{
    daemon_t *d = (daemon_t *) param;

    if (d->client_pid > 0) {
        kill(d->client_pid, SIGKILL);
    }

    if (d->daemon_pid > 0) {
        kill(d->daemon_pid, SIGKILL);
    }

    d->status = S_SPAWNING;
    d->client_pid = -1;
    if (d->client_fd > 0) {
        close(d->client_fd);
        d->client_fd = -1;
    }
    close(d->producer_pipe_fd[PIPE_END_READ]);
    close(d->producer_pipe_fd[PIPE_END_WRITE]);
    close(d->consumer_pipe_fd[PIPE_END_READ]);
    close(d->consumer_pipe_fd[PIPE_END_WRITE]);
    d->start.tv_sec = 0;
    d->start.tv_nsec = 0;
}

// avail_array is an array MAX_DAEMONS long that contains 
//              daemon IDs that are non-busy and non-dead
// count is the number of available daemons
void update_status(int *avail_array, int *count)
{
    int i;

    if (count != NULL) {
        *count = 0;
    }
    st.d_avail = 0;
    st.d_busy = 0;
    st.d_spawning = 0;
    st.d_starting = 0;

    for (i = 0; i < num_daemons; i++) {
        if (d[i].status == S_AVAILABLE) {
            if ((count != NULL) && (avail_array != NULL)) {
                avail_array[*count] = i;
                *count = *count + 1;
            }
            st.d_avail++;
        } else if (d[i].status & S_BUSY) {
            st.d_busy++;
        } else if (d[i].status & S_SPAWNING) {
            st.d_spawning++;
        } else if (d[i].status & S_STARTING) {
            st.d_starting++;
        }
    }

    if (st.d_avail == 0) {
        all_busy = 1;
    }
}
