#define DEBUG
#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define END_A 2.1
#define END_B 5.9

#define HANDLE_ERROR( msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while ( 0)

#define HANDLE_ERROR_EN( msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while ( 0)

#define FORWARD_ERROR( msg) \
    do { int en = errno; fprintf( stderr, msg); errno = en; return -1; } while ( 0)

struct ends_t
{
    double end_a;
    double end_b;
};

struct thread_info
{
    pthread_t      tid;    // Thread ID returned by pthread_create().
    int            tnum;   // App-defined thread #.
    struct ends_t* ends;   // Ends of calculating.
    double         result; // Result of calculating.
};

struct thread_info* tinfo = NULL; // Array with threads' info

int          get_num_of_threads( char* str);
int          init_tinfo( int num_of_threads);
static void* calculate( void* arg);

int main( int argc, char* argv[])
{
    int    num_of_threads = 0;
    double result = 0.;

    if ( ( num_of_threads = get_num_of_threads( argv[1])) == -1)
        HANDLE_ERROR( "In get_num_of_threads");

    if ( init_tinfo( num_of_threads) == -1)
        HANDLE_ERROR( "In init_tinfo");

    pthread_attr_t attr;
    cpu_set_t      cpuset;

    if ( pthread_attr_init( &attr) != 0)
        HANDLE_ERROR( "In pthread_attr_init()");
 
    for ( int tnum = 0; tnum < num_of_threads; tnum++)
    {
        CPU_ZERO( &cpuset);
        // ( tnum % 4) is for testing.
        CPU_SET( tnum % 4, &cpuset);

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

    exit( EXIT_SUCCESS);
}

/** Simple function which can integrate only one set up function;
 *  calculation is made from ( x = a) to ( x = b). */
static void* calculate( void* arg)
{
    struct thread_info* info = arg;

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

    for ( double x = info->ends->end_a; x <= info->ends->end_b; x += dx)
    {
        info->result += ( 1. / x) * ( 1. / ( x - 0.5)) * dx;

        if ( errno)
            HANDLE_ERROR( "While calculation\n");
    }
}
// For testing.
int get_num_of_threads( char* str)
{
    return 5;
}

int init_tinfo( int num_of_threads)
{
    tinfo = ( struct thread_info*)calloc( num_of_threads, sizeof( *tinfo));

    if ( tinfo == NULL)
        FORWARD_ERROR( "Calloc tinfo\n");

    double step = ( END_B - END_A) / num_of_threads;
    
    if ( errno)
        FORWARD_ERROR( "Division error\n");


    for ( int i = 0; i < num_of_threads; i++)
    {
        tinfo[i].tnum = i + 1;
        tinfo[i].result = 0;
        tinfo[i].ends = ( struct ends_t*)calloc( 1, sizeof( *tinfo[i].ends));

        if ( tinfo[i].ends == NULL)
            FORWARD_ERROR( "Calloc ends\n");
        
        tinfo[i].ends->end_a = END_A + step * i;
        tinfo[i].ends->end_b = END_A + step * ( i + 1);
    }

    return 0;
}
