#ifndef __SCALER_H__
#define __SCALER_H__

#define DEBUG
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define END_A 2.1
#define END_B 5.9
#define MAX_STR_LENGTH 128
#define MAX_CPUS 128
#define MAX_THREADS_NUM 1024
#define HT_SPEEDUP 1.2

#define FUNC( x) ( ( 0.23 * x * x) - ( x - 0.5))

#define HANDLE_ERROR( msg)        \
    do {                          \
        perror(msg);              \
        free_mem( tinfo, &sinfo); \
        exit(EXIT_FAILURE);       \
    } while ( 0)

#define HANDLE_ERROR_EN( msg, en) \
    do {                          \
        errno = en;               \
        perror(msg);              \
        free_mem( tinfo, &sinfo); \
        exit(EXIT_FAILURE);       \
    } while ( 0)

#define FORWARD_ERROR( msg)    \
    do {                       \
        int en = errno;        \
        fprintf( stderr, msg); \
        errno = en;            \
        return -1;             \
    } while ( 0)

#define FORWARD_ERROR_EN( msg, en) \
    do {                           \
        fprintf( stderr, msg);     \
        errno = en;                \
        return -1;                 \
    } while ( 0)

#define MIN( a, b) ( a > b ? b : a)

#define MAX( a, b) ( a > b ? a : b)

typedef struct ends
{
    double end_a;
    double end_b;
} ends_t;

typedef struct thread_info
{
    pthread_t      tid;     // Thread ID returned by pthread_create().
    int            tnum;    // App-defined thread #.
    int            cpu;     // Set CPU.
    int            core_id; // Core ID.
    ends_t         ends;    // Ends of calculating.
    double         result;  // Result of calculating.
} thread_info_t;

typedef struct cpu
{
    int     cnum;
    int     num_of_threads;
    double  local_step;
    ends_t  ends;
} cpu_t;

typedef struct core
{
    int    num_of_cpus;
    int    uses_hyperthreading;
    int    num_of_threads;
    cpu_t  cpus[2];
    
} core_t;

typedef struct sys_info
{
    int     num_of_cpus;
    int     num_of_online_cpus;
    int     num_of_cores;
    int     num_of_single_cores;
    int     num_of_double_cores;
    core_t* cores;
} sys_info_t;

int          get_num_of_threads( char* str);
int          init_tinfo( char* num_of_threads_str,
                         thread_info_t**, sys_info_t*);
int          init_sysinfo( sys_info_t*);
void         free_mem();

#endif // __SCALER_H__

