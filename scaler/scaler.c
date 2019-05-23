//#define DEBUG
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

#define MIN( a, b) a > b ? b : a

#define MAX( a, b) a > b ? a : b

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

int            num_of_threads = 0;
int            num_of_allocated_threads = 0; // To avoid memory leaks.
thread_info_t* tinfo = NULL;                 // Array with threads' info.
sys_info_t     sinfo = {0, 0, 0, 0, 0, NULL};

int          get_num_of_threads( char* str);
int          get_cpu( int core_num, int cpu_in_core_num);
int          init_tinfo( char* num_of_threads_str);
int          init_sysinfo();
void         free_mem();
static void* calculate( void* arg);

int main( int argc, char* argv[])
{
    double result = 0.;

    if ( init_sysinfo() == -1)
        HANDLE_ERROR( "In init_sysinfo()");

    if ( init_tinfo( argv[1]) == -1)
        HANDLE_ERROR( "In init_tinfo");

    pthread_attr_t attr;
    cpu_set_t      cpuset;

    CPU_ZERO( &cpuset);

    if ( pthread_attr_init( &attr) != 0)
        HANDLE_ERROR( "In pthread_attr_init()");

    for ( int tnum = 0; tnum < num_of_threads; tnum++)
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
        result += ( 1. / x) * ( 1. / ( x - 0.5)) * dx;

        if ( errno)
            HANDLE_ERROR( "While calculation\n");
    }

    info->result = result;
}

int get_num_of_threads( char* str)
{
    char* endptr = NULL;

    int n_of_threads = strtol( str, &endptr, 10);

    if ( str == NULL || *endptr != '\0')
        FORWARD_ERROR_EN( "In strtol()\n", EINVAL);

    else if (errno)
        FORWARD_ERROR( "In strtol()\n");

    return n_of_threads;
}

int get_core_id( int core_num)
{
    int core_id = 0;

    while ( sinfo.cores[core_id].num_of_cpus == 0)
        core_id = ( core_id + 1) % MAX_CPUS;

    for ( int i = 0; i < core_num; i++)
    {
        core_id = ( core_id + 1) % MAX_CPUS;

        while ( sinfo.cores[core_id].num_of_cpus == 0)
            core_id = ( core_id + 1) % MAX_CPUS;
    }

    return core_id;
}

int init_tinfo( char* num_of_threads_str)
{
    if ( ( num_of_threads = get_num_of_threads( num_of_threads_str)) == -1)
        FORWARD_ERROR( "In get_num_of_threads()");

    tinfo = ( thread_info_t*)calloc( num_of_threads, sizeof( *tinfo));

    if ( tinfo == NULL)
        FORWARD_ERROR( "Calloc tinfo\n");
    
    int    num_of_used_cpus = 0;

    if ( num_of_threads > sinfo.num_of_cores)
    {
        int num_of_used_double_cores =
            MIN( ( num_of_threads - sinfo.num_of_cores),
                 ( sinfo.num_of_double_cores));

        double all_parts = sinfo.num_of_cores + num_of_used_double_cores * 0.5;

        num_of_used_cpus = sinfo.num_of_cores + num_of_used_double_cores;

        int core_num = 0;

        for ( int i = 0; i < num_of_used_double_cores; i++)
        {
            while ( sinfo.cores[get_core_id( core_num)].num_of_cpus != 2)
                core_num++;

            sinfo.cores[get_core_id( core_num++)].uses_hyperthreading = 1;
        }

        double end_a = END_A;
        double step = ( END_B - END_A) / all_parts;

        for ( int i = 0; i < sinfo.num_of_cores; i++)
        {
            core_t* core = &sinfo.cores[get_core_id( i)];

            core->num_of_threads = num_of_threads / sinfo.num_of_cores
                                + ( i < ( num_of_threads % sinfo.num_of_cores));

            core->cpus[0].ends.end_a = end_a;
            
            if ( core->uses_hyperthreading)
            {
                core->cpus[0].ends.end_b = end_a + step * 0.75;
                core->cpus[1].ends.end_a = end_a + step * 0.75;
                core->cpus[1].ends.end_b = end_a + step * 1.5;
                end_a += step * 1.5;

                int num_of_threads_1st_cpu = core->num_of_threads / 2
                                             + core->num_of_threads % 2;
                int num_of_threads_2nd_cpu = core->num_of_threads / 2;

                core->cpus[0].local_step = step * 0.75 / num_of_threads_1st_cpu;
                core->cpus[1].local_step = step * 0.75 / num_of_threads_2nd_cpu;

                #ifdef DEBUG
                printf( "core #%d\n"
                        "\tcore id = %u\n"
                        "\tcore uses hyperthreading\n"
                        "\t1st cpu: from %lg to %lg\n"
                        "\t2nd cpu: from %lg to %lg\n",
                        i, get_core_id( i), core->cpus[0].ends.end_a,
                        core->cpus[0].ends.end_b,
                        core->cpus[1].ends.end_a,
                        core->cpus[1].ends.end_b);
                #endif // DEBUG
            }

            else
            {
                core->cpus[0].ends.end_b = end_a + step;
                end_a += step;

                core->cpus[0].local_step = step / core->num_of_threads;
 
                #ifdef DEBUG
                printf( "core #%d\n"
                        "\tcore id = %u\n"
                        "\tcore doesn't use hyperthreading\n"
                        "\tfrom %lg to %lg\n",
                        i, get_core_id( i), core->cpus[0].ends.end_a,
                        core->cpus[0].ends.end_b,
                        core->cpus[1].ends.end_a,
                        core->cpus[1].ends.end_b);
                #endif // DEBUG
            }
        }
    }

    else
    {
        double step = ( END_B - END_A) / num_of_threads;

        for ( int i = 0; i < num_of_threads; i++)
        {
            core_t* core = &sinfo.cores[get_core_id( i)];

            core->num_of_threads = 1;
            core->cpus[0].local_step = step / core->num_of_threads;
            core->cpus[0].ends.end_a = END_A + step * i;
            core->cpus[0].ends.end_b = core->cpus[0].ends.end_a + step;

            #ifdef DEBUG
            printf( "core #%d\n"
                    "\tcore id = %u\n"
                    "\tcore doesn't use hyperthreading\n"
                    "\tfrom %lg to %lg\n",
                    i, get_core_id( i), core->cpus[0].ends.end_a,
                    core->cpus[0].ends.end_b);
            #endif // DEBUG
        }
    }

    int core_num = 0;

    for ( int i = 0; i < num_of_threads; i++)
    {
        tinfo[i].tnum = i + 1;
        tinfo[i].core_id = get_core_id( core_num++);
        tinfo[i].result = 0;

        core_t* core = &sinfo.cores[tinfo[i].core_id];

        int local_thread_num = i / sinfo.num_of_cores;

        int ncpu = -1;

        if ( ( local_thread_num % 2) && core->uses_hyperthreading)
            ncpu = 1;

        else
            ncpu = 0;

        if ( core->uses_hyperthreading)
            local_thread_num /= 2;

        tinfo[i].cpu = core->cpus[ncpu].cnum;
        tinfo[i].ends.end_a = core->cpus[ncpu].local_step * local_thread_num
                              + core->cpus[ncpu].ends.end_a;
        tinfo[i].ends.end_b = tinfo[i].ends.end_a + core->cpus[ncpu].local_step;
    }

    return 0;
}

int init_sysinfo()
{
    int f = open( "/sys/devices/system/cpu/online", O_RDONLY);
    
    if ( f == -1)
        FORWARD_ERROR( "Open 'cpu/online' list\n");

    char* cpu_online = ( char*)calloc( MAX_STR_LENGTH, sizeof( char));

    if ( cpu_online == NULL)
        FORWARD_ERROR( "Calloc()\n");

    ssize_t read_res = read( f, cpu_online, MAX_STR_LENGTH); 

    if ( read_res == -1)
    {
        free( cpu_online);

        FORWARD_ERROR( "Read 'cpu/online' list\n");
    }

    cpu_online[read_res] = 0;

    close( f);

    char is_online[MAX_CPUS];

    if ( is_online == NULL)
    {
        free( cpu_online);

        FORWARD_ERROR( "Calloc()\n");
    }

    char* endptr = cpu_online;

    sinfo.cores = ( core_t*)calloc( MAX_CPUS, sizeof( *sinfo.cores));

    while ( *endptr != '\0')
    {
        char*nptr  = endptr;

        long token_1 = strtol( nptr, &endptr, 10);
        long token_2 = token_1;

        if ( *endptr == '-')
        {
            nptr = endptr + 1;
            
            token_2 = strtol( nptr, &endptr, 10);
        }

        else if ( *endptr != ',' && *endptr != '\0')
        {
            free( cpu_online);

            FORWARD_ERROR_EN( "Invalid token\n", EINVAL);
        }

        if ( *endptr != '\0')
            endptr ++;

        else
            sinfo.num_of_cpus = token_2;

        if ( token_2 >= MAX_CPUS)
        {
            free( cpu_online);

            FORWARD_ERROR_EN( "Too much CPU's\n", EINVAL);
        }

        for ( int i = token_1; i <= token_2; i++)
        {
            sinfo.num_of_online_cpus ++;

            is_online[i] = 1;

            char cpu_topology_path[MAX_STR_LENGTH];

            sprintf( cpu_topology_path,
                     "/sys/devices/system/cpu/cpu%d/topology/core_id", i);

            f = open( cpu_topology_path, O_RDONLY);

            if ( f == -1)
                FORWARD_ERROR( "Open 'cpu/online' list\n");

            char core_id_str[MAX_STR_LENGTH];

            ssize_t read_res = read( f, core_id_str, MAX_STR_LENGTH); 

            if ( read_res == -1)
            {
                free( cpu_online);

                FORWARD_ERROR( "Read 'cpuX/topology/core_id'\n");
            }

            core_id_str[read_res] = 0;

            close( f);

            int core_id = strtol( core_id_str, NULL, 10);

            sinfo.cores[core_id].cpus[sinfo.cores[core_id].num_of_cpus++].cnum = i;

            if ( sinfo.cores[core_id].num_of_cpus == 1)
            {
                sinfo.num_of_cores ++;
                sinfo.num_of_single_cores ++;
            }

            else if ( sinfo.cores[core_id].num_of_cpus == 2)
            {
                sinfo.num_of_single_cores --;
                sinfo.num_of_double_cores ++;
            }

            else
            {
                free( cpu_online);

                FORWARD_ERROR_EN( "Detected more than 2 CPU's per core\n", ENOTSUP);
            }
        }
    }

    free( cpu_online);

    return 0;
}

void free_mem()
{
    if ( tinfo) free( tinfo);

    if ( sinfo.cores) free( sinfo.cores);
}

