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

volatile sig_atomic_t all_busy;

typedef struct {
    unsigned long daemon_spawns;
    unsigned long daemon_deaths;
    unsigned long daemon_respawns;
    unsigned long queries_total;
    unsigned long queries_replied;
    unsigned long queries_delayed;
    unsigned long queries_failed;
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

status_t st;

#endif
