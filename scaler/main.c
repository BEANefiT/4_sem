#include "scaler.h"

#define END_A 2.1
#define END_B 5.9

#define FUNC( x) ( ( 0.23 * x * x) - ( x - 0.5))

static void* calculate( void* arg);

int main( int argc, char* argv[])
{
    int            num_of_threads = 0;
    thread_info_t* tinfo = NULL;                 // Array with threads' info.
    sys_info_t     sinfo = {0, 0, 0, 0, 0, NULL};

    double result = 0.;

    if ( init_sysinfo( &sinfo) == -1)
        HANDLE_ERROR( "In init_sysinfo()");

    if ( ( num_of_threads = init_tinfo( argv[1], &tinfo, &sinfo)) == -1)
        HANDLE_ERROR( "In init_tinfo");

    pthread_attr_t attr;
    cpu_set_t      cpuset;

    CPU_ZERO( &cpuset);

    if ( pthread_attr_init( &attr) != 0)
        HANDLE_ERROR( "In pthread_attr_init()");

    for ( int tnum = 0; tnum < MAX( num_of_threads, sinfo.num_of_cores); tnum++)
    {
        CPU_ZERO( &cpuset);

        CPU_SET( tinfo[tnum].cpu, &cpuset);

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

    printf( "Result is %lg\n", result);

    free_mem( tinfo, &sinfo);

    exit( EXIT_SUCCESS);
}

static void* calculate( void* arg)
{
    thread_info_t* info = arg;

#ifdef DEBUG
    printf( "thread #%d:\n"
            "\tid = %u\n"
            "\tcpu = %d\n"
            "\tend_a = %lg\tend_b = %lg\n",
            info->tnum, info->tid, info->cpu,
            info->ends.end_a, info->ends.end_b);
#endif // DEBUG

    double dx = 1e-9;
    double end_a = info->ends.end_a;
    double end_b = info->ends.end_b;
    double result = 0.;

    for ( double x = end_a; x <= end_b; x += dx)
    {
        result += FUNC( x) * dx;

        if ( errno)
        {
            perror( "While calculation\n");

            break;
        }
    }

    info->result = result;
}

