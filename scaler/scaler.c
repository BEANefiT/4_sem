#define DEBUG
#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define END_A 2.1
#define END_B 9.9
#define MAX_STR_LENGTH 128
#define MAX_CPUS 128

#define HANDLE_ERROR( msg) \
    do { perror(msg); free_mem(); exit(EXIT_FAILURE); } while ( 0)

#define HANDLE_ERROR_EN( msg, en) \
    do { errno = en; perror(msg); free_mem(); exit(EXIT_FAILURE); } while ( 0)

#define FORWARD_ERROR( msg) \
    do { int en = errno; fprintf( stderr, msg); errno = en; return -1; } while ( 0)

#define FORWARD_ERROR_EN( msg, en) \
    do { fprintf( stderr, msg); errno = en; return -1; } while ( 0)

typedef struct ends
{
    double end_a;
    double end_b;
} ends_t;

typedef struct thread_info
{
    pthread_t      tid;    // Thread ID returned by pthread_create().
    int            tnum;   // App-defined thread #.
    ends_t*        ends;   // Ends of calculating.
    double         result; // Result of calculating.
} thread_info_t;

typedef struct cpu_info
{

} cpu_info_t;

typedef struct sys_info
{
    int num_of_cpus;
    char* is_online;
} sys_info_t;

int            num_of_threads = 0;
int            num_of_allocated_threads = 0; // To avoid memory leaks.
thread_info_t* tinfo = NULL;                 // Array with threads' info.
sys_info_t     sinfo = {0, NULL};

int          get_num_of_threads( char* str);
int          get_cpu( int tnum);
int          init_tinfo();
int          init_sysinfo();
void         free_mem();
static void* calculate( void* arg);

int main( int argc, char* argv[])
{
    double result = 0.;

    if ( ( num_of_threads = get_num_of_threads( argv[1])) == -1)
        HANDLE_ERROR( "In get_num_of_threads");

    if ( init_tinfo() == -1)
        HANDLE_ERROR( "In init_tinfo");

    pthread_attr_t attr;
    cpu_set_t      cpuset;

    CPU_ZERO( &cpuset);

    if ( pthread_attr_init( &attr) != 0)
        HANDLE_ERROR( "In pthread_attr_init()");
 
    for ( int tnum = 0; tnum < num_of_threads; tnum++)
    {
        if ( tnum != 0)
            CPU_CLR( get_cpu( tnum - 1), &cpuset);

        CPU_SET( get_cpu( tnum), &cpuset);

        if ( pthread_attr_setaffinity_np( &attr, sizeof( cpu_set_t), &cpuset) != 0)
            HANDLE_ERROR( "In pthread_attr_setaffinity_np()");

        if ( pthread_create( &tinfo[tnum].tid, &attr, &calculate, &(tinfo[tnum])) != 0)
            HANDLE_ERROR( "In pthread_create");
    }

    if ( pthread_attr_destroy( &attr) != 0)
        HANDLE_ERROR( "In pthread_attr_destroy()");

    for ( int tnum = 0; tnum < num_of_threads; tnum++)
    {
        if ( pthread_join( tinfo[tnum].tid, NULL) != 0)
            HANDLE_ERROR( "In pthread_join");

        result += tinfo[tnum].result;
    }

    free_mem();

    exit( EXIT_SUCCESS);
}

/** Simple function which can integrate only one set up function;
 *  calculation is made from ( x = a) to ( x = b). */
static void* calculate( void* arg)
{
    thread_info_t* info = arg;

#ifdef DEBUG
    printf( "thread #%d:\n"
            "\tid = %u\n"
            "\tinitialised result = %lg\n"
            "\tends ptr = %p\n"
            "\tend_a = %lg\tend_b = %lg\n",
            info->tnum, info->tid, info->result, info->ends,
            info->ends->end_a, info->ends->end_b);
#endif // DEBUG

    double dx = 1e-9;
    double end_a = info->ends->end_a;
    double end_b = info->ends->end_b;
    double result = 0.;

    for ( double x = end_a; x <= end_b; x += dx)
    {
        result += ( 1. / x) * ( 1. / ( x - 0.5)) * dx;

        if ( errno)
            HANDLE_ERROR( "While calculation\n");
    }

    info->result = result;
}
// For testing.
int get_num_of_threads( char* str)
{
    return 1;
}

// For testing.
int get_cpu( int tnum)
{
    return tnum % 4 + 1;
}

int init_tinfo()
{
    tinfo = ( thread_info_t*)calloc( num_of_threads, sizeof( *tinfo));

    if ( tinfo == NULL)
        FORWARD_ERROR( "Calloc tinfo\n");

    double step = ( END_B - END_A) / num_of_threads;
    
    if ( errno)
        FORWARD_ERROR( "Division error\n");


    for ( int i = 0; i < num_of_threads; i++)
    {
        tinfo[i].tnum = i + 1;
        tinfo[i].result = 0;
        tinfo[i].ends = ( ends_t*)calloc( 1, sizeof( *tinfo[i].ends));

        if ( tinfo[i].ends == NULL)
            FORWARD_ERROR( "Calloc ends\n");
        
        num_of_allocated_threads ++;

        tinfo[i].ends->end_a = END_A + step * i;
        tinfo[i].ends->end_b = END_A + step * ( i + 1);
    }

    return 0;
}

int init_sysinfo()
{
    int f = open( "/sys/devices/system/cpu/online", O_RDONLY);
    
    if ( f == -1)
        FORWARD_ERROR( "Open 'cpu/online' list\n");

    char cpu_online[MAX_STR_LENGTH] = {};

    ssize_t read_res = read( f, &cpu_online, MAX_STR_LENGTH); 

    if ( read_res == -1)
        FORWARD_ERROR( "Read 'cpu/online' list\n");

    cpu_online[read_res] = 0;

    close( f);

    sinfo.is_online = ( char*)calloc( MAX_CPUS, sizeof( char));

    char* endptr = cpu_online;

    while ( *endptr != '\0')
    {
        cpu_online = endptr;

        long token_1 = strtol( cpu_online, &endptr, 10);
        long token_2 = token_1;

        if ( *endptr == '-')
        {
            cpu_online = endptr + 1;
            
            token_2 = strtol( cpu_online, &endptr, 10);
        }

        else if ( *endptr != ',' && *endptr != '\0')
            FORWARD_ERROR_EN( "Invalid token\n", EINVAL);

        if ( *endptr != '\0')
            endptr ++;

        else
            sinfo.num_of_cpus = token2;

        for ( long i = token_1; i <= token_2; i++)
            sinfo.is_online[i] = 1;
    }
}

void free_mem()
{
    for ( int i = 0; i < num_of_allocated_threads; i++)
        free( tinfo[i].ends);

    if ( tinfo) free( tinfo);

    if ( sinfo.is_online) free( sinfo.is_online);
}

