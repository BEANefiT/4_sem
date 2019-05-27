#include "scaler.h"

#define HANDLE_ERROR( msg)        \
    do {                          \
        perror(msg);              \
        free_mem( &sinfo);        \
        if ( tinfo) free( tinfo); \
        exit(EXIT_FAILURE);       \
    } while ( 0)

#define HANDLE_ERROR_EN( msg, en) \
    do {                          \
        errno = en;               \
        perror(msg);              \
        free_mem( &sinfo);        \
        if ( tinfo) free( tinfo); \
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

#define CHECK( func) \
    do { if ( func == -1) HANDLE_ERROR( "In "#func); } while ( 0)

#define CHECK_FORWARD( func) \
    do { if ( func == -1) FORWARD_ERROR( "In "#func); } while ( 0)

#define MIN( a, b) ( a > b ? b : a)

#define MAX( a, b) ( a > b ? a : b)

#define END_A 2.1
#define END_B 5.9

#define FUNC( x) ( ( 0.23 * x * x) - ( x - 0.5))

typedef struct
{
    double end_a;
    double end_b;
} ends_t;

typedef struct
{
    pthread_t tid;       // Thread ID returned by pthread_create().
    int       tnum;      // App-defined thread #.
    int       core_id;   // Set Core ID.
    int       cpu_id;    // Set CPU ID.
    double    partition; // Thread's part of total execution.
    ends_t    ends;      // Ends of calculating.
    double    result;    // Result of calculating.
} thread_module_t;

int          create_threads();
static void* calculate( void* arg);

static int              nthreads = -1;
static double           result   = 0.;
static sys_info_t       sinfo    = {0, 0, 0, 0, 0, 0., NULL};
static thread_module_t* tinfo    = NULL;

int main( int argc, char* argv[])
{
    if ( argc != 2)
    {
        fprintf( stderr, "Usage: %s nthreads\n", argv[0]);
        exit( EXIT_FAILURE);
    }

    CHECK( init_sysinfo( &sinfo));

    CHECK( ( nthreads = get_nthreads( argv[1])));

    tinfo = ( thread_module_t*)calloc( nthreads, sizeof( *tinfo));

    if ( tinfo == NULL)
        HANDLE_ERROR( "In calloc of tinfo");

    CHECK( init_tinfo( nthreads, ( thread_info_t*)tinfo, &sinfo));

    CHECK( create_threads());

    for ( int tnum = 0; tnum < nthreads; tnum++)
    {
        if ( pthread_join( tinfo[tnum].tid, NULL) != 0)
            HANDLE_ERROR( "In pthread_join");

        result += tinfo[tnum].result;
    }

    printf( "Result is %lg\n", result);

    free_mem( &sinfo);

    exit( EXIT_SUCCESS);
}

int create_threads()
{
    pthread_attr_t attr;
    cpu_set_t      cpuset;

    if ( pthread_attr_init( &attr) != 0)
        FORWARD_ERROR( "In pthread_attr_init()\n");

    double end_a = END_A;
    double step = ( END_B - END_A) / sinfo.partition;

    for ( int tnum = 0; tnum < MAX( nthreads, sinfo.num_of_cores); tnum++)
    {
        tinfo[tnum].ends.end_a = end_a;
        tinfo[tnum].ends.end_b = end_a;

        if ( tinfo[tnum].partition != 0.)
        {
            end_a += step * tinfo[tnum].partition;
            tinfo[tnum].ends.end_b = end_a;
        }

        else
        {
            tinfo[tnum].ends.end_a = END_A;
            tinfo[tnum].ends.end_b = END_B;
        }
        
        CPU_ZERO( &cpuset);

        CPU_SET( tinfo[tnum].cpu_id, &cpuset);

        if ( pthread_attr_setaffinity_np( &attr, sizeof( cpu_set_t),
                                          &cpuset) != 0)
            HANDLE_ERROR( "In pthread_attr_setaffinity_np()");

        if ( pthread_create( &tinfo[tnum].tid, &attr, &calculate,
                             &(tinfo[tnum])) != 0)
            HANDLE_ERROR( "In pthread_create");
    }

    if ( pthread_attr_destroy( &attr) != 0)
        HANDLE_ERROR( "In pthread_attr_destroy()");

    return 0;
}

static void* calculate( void* arg)
{
    thread_module_t* info = arg;

#ifdef DEBUG
    printf( "thread #%d:\n"
            "\tid = %u\n"
            "\tcpu = %d\n"
            "\tend_a = %lg\tend_b = %lg\n",
            info->tnum, info->tid, info->cpu_id,
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

#undef END_A
#undef END_B
#undef FUNC

#undef HANDLE_ERROR
#undef HANDLE_ERROR_EN
#undef FORWARD_ERROR
#undef FORWARD_ERROR_EN
#undef CHECK
#undef CHECK_FORWARD
#undef MIN
#undef MAX

