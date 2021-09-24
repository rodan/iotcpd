#ifndef __MAIN_H_
#define __MAIN_H_

#include <inttypes.h>

#define MAX_DAEMONS 64
#define MSG_MAX 4096

#ifdef MAIN_LEVEL
    #define MAIN_EXPORT
#else
    #define MAIN_EXPORT extern 
#endif

///////////////////////////////
// default values that can be changed via 
// command line arguments
//

MAIN_EXPORT int debug;
MAIN_EXPORT int num_daemons;
MAIN_EXPORT int port;
MAIN_EXPORT int busy_timeout;
MAIN_EXPORT int alarm_interval;
MAIN_EXPORT char *ip4;
MAIN_EXPORT char *ip6;
MAIN_EXPORT char *daemon_str;
MAIN_EXPORT char **daemon_array;

///////////////////////////////

MAIN_EXPORT volatile sig_atomic_t all_busy;
MAIN_EXPORT char *daemon_array_container;

typedef struct {
    unsigned long daemon_spawns;
    unsigned long queries_total;
    unsigned long queries_replied;
    unsigned long queries_delayed;
    unsigned long queries_timeout;
    unsigned long queries_failed;
    unsigned long queries_unknown;
    unsigned long queries_0_100;
    unsigned long queries_100_250;
    unsigned long queries_250_500;
    unsigned long queries_500_750;
    unsigned long queries_750_1000;
    unsigned long queries_1000;
    unsigned int d_avail;
    unsigned int d_busy;
    unsigned int d_spawning;
    unsigned int d_starting;
    time_t started;
} status_t;

MAIN_EXPORT status_t st;

#endif
