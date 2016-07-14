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
    volatile unsigned long queries_total;
    volatile unsigned long queries_replied;
    volatile unsigned long queries_delayed;
    volatile unsigned long queries_failed;
    volatile unsigned long queries_0_100;
    volatile unsigned long queries_100_250;
    volatile unsigned long queries_250_500;
    volatile unsigned long queries_500_750;
    volatile unsigned long queries_750_1000;
    volatile unsigned long queries_1000;
    unsigned int d_avail;
    unsigned int d_busy;
    unsigned int d_dead;
    unsigned int d_restarting;
    time_t started;
} status_t;

status_t st;

#endif
