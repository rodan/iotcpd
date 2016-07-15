
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include "config.h"
#include "main.h"
#include "daemon_glue.h"

char *get_ip_str(const struct sockaddr *sa, char *dst, const size_t maxlen)
{
    switch (sa->sa_family) {
    case AF_INET:
        inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), dst, maxlen);
        break;

    case AF_INET6:
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), dst, maxlen);
        break;

    default:
        strncpy(dst, "Unknown AF", maxlen);
        return NULL;
    }

    return dst;
}

static int make_socket_non_blocking(int sfd)
{
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl failed");
        return EXIT_FAILURE;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int io_handler(const int fd)
{
    char buff_rx[MSG_MAX], buff_tx[MSG_MAX];
    ssize_t bytes;
    ssize_t count;
    int s;
    pid_t pid;
    int sel_daemon;
    int avail_daemon[MAX_DAEMONS];
    int j;
    int timer;
    struct timespec start;
    struct sigaction sa;

    count = read(fd, buff_rx, (sizeof buff_rx) - 1);
    if (count == -1) {
        // If errno == EAGAIN, that means we have read all
        // data. So go back to the main loop
        if (errno != EAGAIN) {
            perror("read failure");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    } else if (count == 0) {
        // end of file. the remote has closed the connection
        return EXIT_FAILURE;
    }

    buff_rx[count] = 0;

#ifdef CONFIG_DEBUG
    /*
    // write the buffer to standard output
    s = write(1, buff_rx, count);
    if (s == -1) {
        perror("write to stdout failed");
        return EXIT_FAILURE;
    }
    */
#endif

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    // select a daemon that is available
    st.queries_total++;
    update_status(&avail_daemon[0], &j);

    if (j == 0) {
        // no available daemons
        st.queries_delayed++;

        timer = 0;
        while (timer < 800) {
            usleep(1000);
            timer++;
            if (!all_busy) {
                break;
            }
        }

        //printf("timer %d, busy %d\n", timer, all_busy);
        if (all_busy) {
            st.queries_failed++;
            s = write(fd, "ERR\n", 4);
            close(fd);
            fprintf(stderr, "dropping connection. avail/busy/spawning/starting daemons: %d/%d/%d/%d\n", st.d_avail, st.d_busy, st.d_spawning, st.d_starting);
            return EXIT_FAILURE;
        } else {
            // at least one daemon is available, lets select one
            update_status(&avail_daemon[0], &j);
        }
    }

    sel_daemon = avail_daemon[rand() % j];
    d[sel_daemon].status = S_BUSY;
    d[sel_daemon].client_fd = fd;
    d[sel_daemon].start.tv_sec = start.tv_sec;
    d[sel_daemon].start.tv_nsec = start.tv_nsec;

    asm( "nop" );
    // fork and send string to one of the daemons
    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "fork error '%s (%d)'\n", strerror(errno), errno);
        return EXIT_FAILURE;
    } else if (pid > 0) {
        // parent
        d[sel_daemon].client_pid = pid;
        st.queries_replied++;
#ifdef CONFIG_DEBUG
        printf("pid %d forked\n", pid);
#endif
    } else if (pid == 0) {
        // child

        // remove signal handlers that were set in the parent
        sa.sa_handler = SIG_IGN;
        sigaction(SIGCHLD, &sa, NULL);
        sigaction(SIGALRM, &sa, NULL);
        sigaction(SIGUSR1, &sa, NULL);
        sigaction(SIGUSR2, &sa, NULL);
        sigaction(SIGHUP, &sa, NULL);

        // "stay awhile and listen"
        // in case this child dies too soon the cleanup will not find the pid set by
        // the parent a few lines above
        usleep(1000);

        bytes = write(d[sel_daemon].producer_pipe_fd[PIPE_END_WRITE], buff_rx, strlen(buff_rx));
        if (bytes == -1) {
            fprintf(stderr, "pipe write error '%s (%d)'\n", strerror(errno), errno);
            return EXIT_FAILURE;
        }

        bytes = read(d[sel_daemon].consumer_pipe_fd[PIPE_END_READ], buff_tx, MSG_MAX);
        if (bytes == -1) {
            fprintf(stderr, "pipe read error '%s (%d)'\n", strerror(errno), errno);
            return EXIT_FAILURE;
        }

        buff_tx[bytes] = '\0';

        if (bytes == -1) {
            fprintf(stderr, "pipe write error '%s (%d)'\n", strerror(errno), errno);
            return EXIT_FAILURE;
        }
        // write the buffer to remote fd
        s = write(fd, buff_tx, bytes);
        if (s == -1) {
            perror("write to remote fd failed");
            return EXIT_FAILURE;
        }

#ifdef CONFIG_DEBUG
        printf("d[%d] sent %lub to fd %d. %d/%d busy\n", sel_daemon, bytes, fd, st.d_busy, num_daemons);
#endif
        _exit(0);
    }

    return EXIT_SUCCESS;
}

void network_glue(void)
{
    struct sockaddr_in s4;
    struct sockaddr_in6 s6;
    struct sockaddr_storage ss;
    int sfd, s;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;

    if (strlen(ip4) > 0) {
        s4.sin_family = AF_INET;
        inet_pton(AF_INET, ip4, &(s4.sin_addr));
        s4.sin_port = htons(port);
        memcpy(&ss, &s4, sizeof(s4));
    } else if (strlen(ip6) > 0) {
        s6.sin6_family = AF_INET6;
        inet_pton(AF_INET6, ip6, &(s6.sin6_addr));
        s6.sin6_port = htons(port);
        memcpy(&ss, &s6, sizeof(s6));
    } else {
        fprintf(stderr, "IPv4/IPv6 unspecified\n");
        return;
    }

    sfd = socket(ss.ss_family, SOCK_STREAM, 0);

    if (sfd == -1) {
        perror("socket failed");
        return;
    }

    make_socket_non_blocking(sfd);

#ifndef WIN32
    int one = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
        perror("setsockopt failed");
        return;
    }
#endif

    if (bind(sfd, (struct sockaddr *)&ss, sizeof(ss)) < 0) {
        perror("bind failed");
        return;
    }

    if (listen(sfd, num_daemons) < 0) {
        perror("listen failed");
        return;
    }

    efd = epoll_create1(0);
    if (efd == -1) {
        perror("epoll_create");
        abort();
    }

    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    if (s == -1) {
        perror("epoll_ctl");
        abort();
    }

    events = calloc(num_daemons, sizeof event);

    // the event loop
    while (1) {
        int n, i;

        n = epoll_wait(efd, events, num_daemons, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
                // an error has occured on this fd, or the socket is not
                // ready for reading (why were we notified then?)
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            }

            else if (sfd == events[i].data.fd) {
                // we have a notification on the listening socket, which
                // means one or more incoming connections.
                while (1) {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;

                    in_len = sizeof in_addr;
                    infd = accept(sfd, &in_addr, &in_len);
                    if (infd == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            // we have processed all incoming
                            // connections.
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }
#ifdef CONFIG_DEBUG
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                    s = getnameinfo(&in_addr, in_len,
                                    hbuf, sizeof hbuf,
                                    sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);
                    if (s == 0) {
                        printf("new connection - fd %d, host %s, port %s\n", 
                                infd, hbuf, sbuf);
                    }
#endif

                    // make the incoming socket non-blocking and add it to the
                    // list of fds to monitor.
                    s = make_socket_non_blocking(infd);
                    if (s == -1) {
                        abort();
                    }

                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;
            } else {
                // data is present on the fd and it's ready to be read
                io_handler(events[i].data.fd);
            }
        }
    }

    free(events);
    close(sfd);
}
