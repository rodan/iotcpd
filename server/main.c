
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/wait.h>
#include <time.h>

#include "config.h"

#define MAIN_LEVEL
#include "main.h"
#include "networking.h"
#include "daemon_glue.h"
#include "helpers.h"
#include "version.h"

void show_version()
{
    fprintf(stdout, "iotcpd v%d.%db%d\n", VER_MAJOR, VER_MINOR, BUILD);
}

void show_help()
{
    fprintf(stdout, "Usage: iotcpd [OPTION]\n");
    fprintf(stdout, "Redirector daemon that allows for multiple identical non-networking daemons \n");
    fprintf(stdout, "that talk via stdin/stdout and only exit on EOF to be bound to a TCP port.\n");
    fprintf(stdout, "Mandatory arguments to long options are mandatory for short options too.\n\n");
    fprintf(stdout,
            "\t-h, --help\n"
            "\t\tthis help\n"
            "\t-d, --daemon=NAME\n"
            "\t\tdaemon to be run - (default '%s')\n"
            "\t-i, --ipv4=IP\n"
            "\t\tIPv4 used for listening for connections - (default '%s')\n"
            "\t-I, --ipv6=IP\n"
            "\t\tIPv6 used for listening for connections - (default '%s')\n"
            "\t-p, --port=NUM\n"
            "\t\tport used - (default '%d')\n"
            "\t-n, --num-daemons=NUM\n"
            "\t\tnumber of daemons spawned - (default '%d')\n"
            "\t-b, --busy-timeout=NUM\n"
            "\t\tnumber of seconds after which an unresponsive daemon is restarted - (default '%d')\n"
            "\t-a, --alarm-interval=NUM\n"
            "\t\ttime interval in seconds between two consecutive ALARM interrupts - (default '%d')\n\n"
            "Example:\n"
            "\tiotcpd --num-daemons 8 --daemon \"squidGuard -c sg/adblock.conf\" \\\n"
            "\t\t--ipv4 10.20.30.40 --port 1234\n", daemon_str, ip4, ip6, port, num_daemons, busy_timeout, alarm_interval);
}

void signal_handler(int sig, siginfo_t * si, void *context)
{
    int status;
    pid_t pid;
    int i;
    sigset_t x;
    struct timespec now, diff;
    long long ldiff;

    if (sig == SIGCHLD) {
        sigemptyset(&x);
        sigaddset(&x, SIGCHLD);
        sigprocmask(SIG_BLOCK, &x, NULL);

        //if (si->si_code == CLD_EXITED || si->si_code == CLD_KILLED) {
            while (1) {
                pid = waitpid(-1, &status, WNOHANG);
                if (pid > 0) {
#ifdef CONFIG_VERBOSE
                    printf("sigchld %d, status %d\n", pid, status);
#endif
                    for (i = 0; i < num_daemons; i++) {
                        if (d[i].client_pid == pid) {
                            // close connection with client
                            close(d[i].client_fd);
                            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
                            diff = ts_diff(d[i].start, now);
                            ldiff = (diff.tv_sec * 1000) + (diff.tv_nsec / 1000000);
                            if (ldiff < 100) {
                                st.queries_0_100++;
                            } else if (ldiff < 250) {
                                st.queries_100_250++;
                            } else if (ldiff < 500) {
                                st.queries_250_500++;
                            } else if (ldiff < 750) {
                                st.queries_500_750++;
                            } else if (ldiff < 1000) {
                                st.queries_750_1000++;
                            } else {
                                st.queries_1000++;
                            }
#ifdef CONFIG_VERBOSE
                            printf("d[%d] pid %d exited after %lld ms, fd %d closed\n", i, pid,
                                   ldiff, d[i].client_fd);
#endif
                            d[i].client_fd = -1;
                            d[i].client_pid = -1;
                            if (d[i].status == S_BUSY) {
                                d[i].status = S_AVAILABLE;
                                all_busy = 0;
                            }
                            d[i].start.tv_sec = 0;
                            d[i].start.tv_nsec = 0;
                        } else if (d[i].daemon_pid == pid) {
                            fprintf(stderr, "daemon d[%d] has died\n", i);
                            d[i].daemon_pid = -1;
                            frag(&d[i]);
                        }
                    }
                } else {
                    // =  0 if nothing else left to reap
                    // = -1 if No child processes
                    sigprocmask(SIG_UNBLOCK, &x, NULL);
                    return;
                }
            }
        //}
        sigprocmask(SIG_UNBLOCK, &x, NULL);
    } else if (sig == SIGINT) {
        network_free();
        free(daemon_array_container);
        free(daemon_array);
        free(d);
        _exit(EXIT_SUCCESS);
    } else if (sig == SIGUSR1) {
        update_status(NULL, NULL);
        fprintf(stdout, " --- statistics ---- >8 -------\n");
        fprintf(stdout, "queries total      %lu\n", st.queries_total);
        fprintf(stdout, "queries replied    %lu\n", st.queries_replied);
        fprintf(stdout, "queries delayed    %lu\n", st.queries_delayed);
        fprintf(stdout, "queries timeout    %lu\n", st.queries_timeout);
        fprintf(stdout, "queries failed     %lu\n", st.queries_failed);
        fprintf(stdout, "queries unknown    %lu\n", st.queries_unknown);
        fprintf(stdout, "queries 0-100      %lu\n", st.queries_0_100);
        fprintf(stdout, "queries 100-250    %lu\n", st.queries_100_250);
        fprintf(stdout, "queries 250-500    %lu\n", st.queries_250_500);
        fprintf(stdout, "queries 500-750    %lu\n", st.queries_500_750);
        fprintf(stdout, "queries 750-1000   %lu\n", st.queries_750_1000);
        fprintf(stdout, "queries 1000-      %lu\n", st.queries_1000);
        fprintf(stdout, "daemon spawns      %lu\n", st.daemon_spawns);
        fprintf(stdout, "daemon S_AVAILABLE %u\n", st.d_avail);
        fprintf(stdout, "daemon S_BUSY      %u\n", st.d_busy);
        fprintf(stdout, "daemon S_SPAWNING  %u\n", st.d_spawning);
        fprintf(stdout, "daemon S_STARTING  %u\n", st.d_starting);
        fprintf(stdout, "uptime   %lu\n", time(NULL) - st.started);
        fprintf(stdout, "version  %d.%db%d\n", VER_MAJOR, VER_MINOR, BUILD);
        fprintf(stdout, " --- statistics ---- 8< -------\n");
    } else if (sig == SIGUSR2) {
        fprintf(stdout, " --- internal regs - 8< -------\n");
        for (i = 0; i < num_daemons; i++) {
            fprintf(stdout, "d[%d].status = %d\n", i, d[i].status);
            fprintf(stdout, "d[%d].producer_pipe_fd[PIPE_END_WRITE] = %d\n", i,
                    d[i].producer_pipe_fd[PIPE_END_WRITE]);
            fprintf(stdout, "d[%d].consumer_pipe_fd[PIPE_END_READ] = %d\n", i,
                    d[i].consumer_pipe_fd[PIPE_END_READ]);
            fprintf(stdout, "d[%d].daemon_pid = %d\n", i, d[i].daemon_pid);
            fprintf(stdout, "d[%d].client_pid = %d\n", i, d[i].client_pid);
            fprintf(stdout, "d[%d].client_fd = %d\n", i, d[i].client_fd);
            fprintf(stdout, "d[%d].start.tv_sec = %lu\n", i, d[i].start.tv_sec);
            fprintf(stdout, "d[%d].start.tv_nsec = %lu\n", i, d[i].start.tv_nsec);
            fprintf(stdout, "\n");
        }
        fprintf(stdout, " --- internal regs - 8< -------\n");
    } else if (sig == SIGALRM) {
        for (i = 0; i < num_daemons; i++) {
            if (d[i].status == S_SPAWNING) {
                spawn(&d[i]);
            } else if (d[i].status == S_STARTING) {
                d[i].status = S_AVAILABLE;
            } else if (d[i].status == S_BUSY) {
                // daemon supervisor
                if (d[i].start.tv_sec) {
                    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
                    diff = ts_diff(d[i].start, now);
                    if (diff.tv_sec > busy_timeout) {
                        fprintf(stderr, "d[%d] - pid %d is killed for being 'busy' for %lus\n", i,
                                d[i].daemon_pid, diff.tv_sec);
                        frag(&d[i]);
                    }
                }
            }
        }
        alarm(alarm_interval);
    } else if (sig == SIGHUP) {
        // refresh all daemons
        for (i = 0; i < num_daemons; i++) {
            frag(&d[i]);
        }
    }
}

void parse_options(int argc, char **argv)
{
    static const char short_options[] = "hvd:i:I:p:n:b:a:";
    static const struct option long_options[] = {
        {.name = "help",.val = 'h'},
        {.name = "version",.val = 'v'},
        {.name = "daemon",.has_arg = 1,.val = 'd'},
        {.name = "ipv4",.has_arg = 1,.val = 'i'},
        {.name = "ipv6",.has_arg = 1,.val = 'I'},
        {.name = "port",.has_arg = 1,.val = 'p'},
        {.name = "num-daemons",.has_arg = 1,.val = 'n'},
        {.name = "busy-timeout",.has_arg = 1,.val = 'b'},
        {.name = "alarm-interval",.has_arg = 1,.val = 'a'},
        {0, 0, 0, 0}
    };
    int option;

    // default values
    daemon_str = "squidGuard -c sg/adblock.conf";
    ip4 = "127.0.0.1";
    ip6 = "";
    port = 9991;
    num_daemons = 4;
    busy_timeout = 15;
    alarm_interval = 2;
    debug = 0;

    if (argc < 2) {
        show_help();
        exit(EXIT_SUCCESS);
    }

    while ((option = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (option) {
        case 'h':
            show_help();
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            show_version();
            exit(EXIT_SUCCESS);
            break;
        case 'd':
            daemon_str = optarg;
            break;
        case 'i':
            ip4 = optarg;
            break;
        case 'I':
            ip6 = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            if (port < 1 || port > 65535) {
                fprintf(stderr, "invalid port value\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'n':
            num_daemons = atoi(optarg);
            if (num_daemons < 1 || num_daemons > MAX_DAEMONS) {
                fprintf(stderr, "invalid num_daemons value\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'b':
            busy_timeout = atoi(optarg);
            if (busy_timeout < 3) {
                fprintf(stderr, "invalid busy_timeout value\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'a':
            alarm_interval = atoi(optarg);
            if (alarm_interval < 1) {
                fprintf(stderr, "invalid alarm_interval value\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            fprintf(stderr, "unknown option: %c\n", option);
            exit(EXIT_FAILURE);
        }
    }

}

int main(int argc, char **argv)
{
    int i;

    char *p;
    int elem = 0;
    struct sigaction sa;
    struct timespec ts;

    setvbuf(stdout, NULL, _IOLBF, 0);
    memset(&st, 0, sizeof(status_t));
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    st.started = time(NULL);

    parse_options(argc, argv);

    daemon_array_container = strdup(daemon_str);
    p = strtok(daemon_array_container, " ");
    daemon_array = (char **)malloc(strlen(daemon_str) + 1);
    while (p) {
        daemon_array[elem] = p;
        p = strtok(NULL, " ");
        elem++;
    }
    daemon_array[elem] = 0;

    d = (daemon_t *) malloc(num_daemons * sizeof(daemon_t));
    for (i = 0; i < num_daemons; i++) {
        d[i].status = S_SPAWNING;
        d[i].daemon_pid = -1;
        d[i].client_pid = -1;
        d[i].client_fd = -1;
        d[i].start.tv_sec = 0;
        d[i].start.tv_nsec = 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_nsec);

    alarm(alarm_interval);

    // networking loop
    network_glue();

    // cleanup
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    for (i = 0; i < num_daemons; i++) {
        frag(&d[i]);
    }

    network_free();
    free(daemon_array_container);
    free(daemon_array);
    free(d);

    return EXIT_SUCCESS;
}
