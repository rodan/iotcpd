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
        d->status = S_AVAILABLE;
        st.daemon_spawns++;
    }

    return NULL;
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
    st.d_dead = 0;

    for (i = 0; i < num_daemons; i++) {
        if (d[i].status == S_AVAILABLE) {
            if ((count != NULL) && (avail_array != NULL)) {
                avail_array[*count] = i;
                *count = *count + 1;
            }
            st.d_avail++;
        } else if (d[i].status == S_BUSY) {
            st.d_busy++;
        } else if (d[i].status == S_DEAD) {
            st.d_dead++;
            st.daemon_respawns++;
            spawn(&d[i]);
        }
    }

    if (st.d_busy == num_daemons) {
        all_busy = 1;
    }
}

