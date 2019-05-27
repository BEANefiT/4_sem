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

#define MAX_CPUS 128
#define MAX_THREADS_NUM 1024
#define HT_SPEEDUP 1.2

typedef struct
{
    pthread_t tid;       // Thread ID returned by pthread_create().
    int       tnum;      // App-defined thread #.
    int       core_id;   // Set Core ID.
    int       cpu_id;    // Set CPU ID.
    double    partition; // Thread's part of total execution.
} thread_info_t;

typedef struct
{
    int    cpu_id;    // CPU ID.
    int    nthreads;  // Number of threads proceeding by CPU.
    double partition; // CPU's part of total execution.
} cpu_info_t;

typedef struct
{
    int        ncpus;     // Number of CPUs in Core.
    int        uses_ht;   // Use of hyperthreading.
    int        nthreads;  // Number of threads proceeding by Core.
    cpu_info_t cpus[2];   // CPUs of Core.
    double     partition; // Core's part of total execution.
} core_info_t;

typedef struct
{
    int          num_of_cpus;
    int          num_of_online_cpus;
    int          num_of_cores;
    int          num_of_single_cores;
    int          num_of_double_cores;
    double       partition;
    core_info_t* cores;
} sys_info_t;

int  get_nthreads( char*);
int  init_sysinfo( sys_info_t*);
int  init_tinfo( int nthreads, thread_info_t**, sys_info_t*);

#endif // __SCALER_H__

