#include "scaler.h"

#define MAX_STR_LENGTH 128

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

int get_num_of_threads( char* str);
int init_cpu( int cpu_id, sys_info_t* sinfo);
int get_core_id( int core_num, sys_info_t* sinfo);
int get_core_id_by_prev( int core_id_prev, sys_info_t* sinfo);

/** Initialise system information about CPUs and Cores.
 *  WARNING: this function allocates memory for sinfo->cores.
 */
int init_sysinfo( sys_info_t* sinfo)
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

    char* endptr = cpu_online;

    sinfo->cores = ( core_info_t*)calloc( MAX_CPUS,
                                          sizeof( *sinfo->cores));

    while ( *endptr != '\0')
    {
        char*nptr  = endptr;

        long token_1 = strtol( nptr, &endptr, 10);
        long token_2 = token_1;

        if ( *endptr == '-')
        {
            nptr    = endptr + 1;
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
            sinfo->num_of_cpus = token_2;

        if ( token_2 >= MAX_CPUS)
        {
            free( cpu_online);
            FORWARD_ERROR_EN( "Too much CPU's\n", EINVAL);
        }

        for ( int i = token_1; i <= token_2; i++)
        {
            if ( init_cpu( i, sinfo) == -1)
            {
                free( cpu_online);
                FORWARD_ERROR( "In init_cpu\n");
            }
        }
    }

    free( cpu_online);
    return 0;
}

/** Initialise CPU with id equal to cpu_id;
 *  bind that CPU to Core by info in 'topology/core_id'
 */
int init_cpu( int cpu_id, sys_info_t* sinfo)
{
    sinfo->num_of_online_cpus ++;

    char cpu_topology_path[MAX_STR_LENGTH];
    sprintf( cpu_topology_path,
             "/sys/devices/system/cpu/cpu%d/topology/core_id",
             cpu_id);

    int f = open( cpu_topology_path, O_RDONLY);

    if ( f == -1)
        FORWARD_ERROR( "In open of 'cpuX/topology/core_id'\n");

    char core_id_str[MAX_STR_LENGTH];

    ssize_t read_res = read( f, core_id_str, MAX_STR_LENGTH); 

    if ( read_res == -1)
        FORWARD_ERROR( "In read of 'cpuX/topology/core_id'\n");

    core_id_str[read_res] = 0;
    close( f);

    int core_id = strtol( core_id_str, NULL, 10);
    int ncpus = sinfo->cores[core_id].ncpus++;
    sinfo->cores[core_id].cpus[ncpus].cpu_id = cpu_id;

    if ( ncpus == 0)
    {
        sinfo->num_of_cores ++;
        sinfo->num_of_single_cores ++;
    }

    else if ( ncpus == 1)
    {
        sinfo->num_of_single_cores --;
        sinfo->num_of_double_cores ++;
    }

    else
        FORWARD_ERROR_EN( "Detected more than 2 CPU's per core\n",
                          ENOTSUP);

    return 0;
}

/** Initialise of threads partitions;
 *  Divide threads by Cores and CPUs
 */
int init_tinfo( int nthreads, thread_info_t* tinfo, sys_info_t* sinfo)
{
    int num_of_used_double_cores = 0;

    if ( nthreads > sinfo->num_of_cores)
    {
        num_of_used_double_cores =
                        MIN( ( nthreads - sinfo->num_of_cores),
                             ( sinfo->num_of_double_cores));
        sinfo->partition = sinfo->num_of_cores 
                       + num_of_used_double_cores * ( HT_SPEEDUP - 1);
    }

    else
        sinfo->partition = nthreads;

    int core_id = get_core_id( 0, sinfo);

    for ( int i = 0; i < sinfo_num_of_cores; i++)
    {
        core_info_t* core = &sinfo->cores[core_id];
        core->nthreads = nthreads / sinfo->num_of_cores
                         + ( i < ( nthreads % sinfo->num_of_cores));

        if ( core->ncpus == 2
             && num_of_used_double_cores)
        {
            num_of_used_double_cores --;
            core->uses_ht = 1;
            core->partition = HT_SPPEDUP;
            core->cpus[0].partition = HT_SPEEDUP / 2;
            core->cpus[1].partition = HT_SPEEDUP / 2;
        }

        else
        {
            core->partition = 1.;
            core->cpus[0].partition = 1.;
        }

        core_id = get_core_id_by_prev( core_id, sinfo);
    }

    return 0;
}

        /*double end_a = END_A;
        double step = ( END_B - END_A) / all_parts;

        for ( int i = 0; i < sinfo->num_of_cores; i++)
        {
            core_t* core = &sinfo->cores[get_core_id( i, sinfo)];

            core->num_of_threads = num_of_threads / sinfo->num_of_cores
                               + ( i < ( num_of_threads % sinfo->num_of_cores));

            core->cpus[0].ends.end_a = end_a;
            
            if ( core->uses_hyperthreading)
            {
                core->cpus[0].ends.end_b = end_a + step * HT_SPEEDUP / 2;
                core->cpus[0].num_of_threads = core->num_of_threads / 2
                                               + core->num_of_threads % 2;
                core->cpus[0].local_step = step * HT_SPEEDUP / 2
                                           / core->cpus[0].num_of_threads;

                core->cpus[1].ends.end_a = end_a + step * HT_SPEEDUP / 2;
                core->cpus[1].ends.end_b = end_a + step * HT_SPEEDUP;
		end_a += step * HT_SPEEDUP;
                core->cpus[1].num_of_threads = core->num_of_threads / 2;
                core->cpus[1].local_step = step * HT_SPEEDUP / 2
                                           / core->cpus[1].num_of_threads;

                #ifdef DEBUG
                printf( "Core #%d\n"
                        "\tCore ID = %u\n"
                        "\tCore uses hyperthreading\n"
                        "\tInterval = %lg\n"
                        "\t1st cpu: from %lg to %lg with %d threads\n"
                        "\t2nd cpu: from %lg to %lg with %d threads\n",
                        i, get_core_id( i, sinfo),
                        core->cpus[1].ends.end_b - core->cpus[0].ends.end_a,
                        core->cpus[0].ends.end_a,
                        core->cpus[0].ends.end_b,
                        core->cpus[0].num_of_threads,
                        core->cpus[1].ends.end_a,
                        core->cpus[1].ends.end_b,
                        core->cpus[1].num_of_threads);
                #endif // DEBUG
            }

            else
            {
                core->cpus[0].ends.end_b = end_a + step;
                end_a += step;

                core->cpus[0].num_of_threads = core->num_of_threads;
                core->cpus[0].local_step = step / core->num_of_threads;
 
                #ifdef DEBUG
                printf( "core #%d\n"
                        "\tcore id = %u\n"
                        "\tcore doesn't use hyperthreading\n"
                        "\tinterval = %lg\n"
                        "\tfrom %lg to %lg with %d threads\n",
                        i, get_core_id( i, sinfo),
                        core->cpus[0].ends.end_b - core->cpus[0].ends.end_a,
                        core->cpus[0].num_of_threads,
                        core->cpus[0].ends.end_a,
                        core->cpus[0].ends.end_b);
                #endif // DEBUG
            }
        }*/
/*
    else
    {
        double step = ( END_B - END_A) / num_of_threads;

        for ( int i = 0; i < sinfo->num_of_cores; i++)
        {
            core_t* core = &sinfo->cores[get_core_id( i, sinfo)];

            core->num_of_threads = 1;
            
            if ( i < num_of_threads)
            {
                core->cpus[0].num_of_threads = core->num_of_threads;
                core->cpus[0].local_step = step / core->num_of_threads;
                core->cpus[0].ends.end_a = END_A + step * i;
                core->cpus[0].ends.end_b = core->cpus[0].ends.end_a + step;
            }

            else
            {
                core->cpus[0].num_of_threads = -1;
                core->cpus[0].local_step = END_B - END_A;
                core->cpus[0].ends.end_a = END_A;
                core->cpus[0].ends.end_b = END_B;
            }

            #ifdef DEBUG
            printf( "core #%d\n"
                    "\tcore id = %u\n"
                    "\tcore doesn't use hyperthreading\n"
                    "\tinterval = %lg\n"
                    "\tfrom %lg to %lg with %d threads\n",
                    i, get_core_id( i, sinfo),
                    core->cpus[0].ends.end_b - core->cpus[0].ends.end_a,
                    core->cpus[0].num_of_threads,
                    core->cpus[0].ends.end_a,
                    core->cpus[0].ends.end_b);
            #endif // DEBUG
        }
    }

    int core_num = 0;

    for ( int i = 0; i < MAX( num_of_threads, sinfo->num_of_cores); i++)
    {
        ( *tinfo)[i].tnum = i + 1;
        ( *tinfo)[i].result = 0;

        if ( i != 0)
            ( *tinfo)[i].core_id =
                        get_core_id_by_prev( ( *tinfo)[i - 1].core_id, sinfo);

        else
            ( *tinfo)[i].core_id = get_core_id( 0, sinfo);

        core_t* core = &sinfo->cores[( *tinfo)[i].core_id];

        int local_thread_num = i / sinfo->num_of_cores;

        int ncpu = -1;

        if ( ( local_thread_num % 2) && core->uses_hyperthreading)
            ncpu = 1;

        else
            ncpu = 0;

        if ( core->uses_hyperthreading)
            local_thread_num /= 2;

        ( *tinfo)[i].cpu = core->cpus[ncpu].cnum;
        ( *tinfo)[i].ends.end_a = core->cpus[ncpu].local_step * local_thread_num
                              + core->cpus[ncpu].ends.end_a;
        ( *tinfo)[i].ends.end_b = ( *tinfo)[i].ends.end_a
                                  + core->cpus[ncpu].local_step;
    }*/

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

int get_core_id_by_prev( int core_id_prev, sys_info_t* sinfo)
{
    int core_id = ( core_id_prev + 1) % MAX_CPUS;

    while ( sinfo->cores[core_id].num_of_cpus == 0)
        core_id = ( core_id + 1) % MAX_CPUS;

    return core_id;
}

int get_core_id( int core_num, sys_info_t* sinfo)
{
    int core_id = 0;

    while ( sinfo->cores[core_id].num_of_cpus == 0)
        core_id = ( core_id + 1) % MAX_CPUS;

    for ( int i = 0; i < core_num; i++)
    {
        core_id = ( core_id + 1) % MAX_CPUS;

        while ( sinfo->cores[core_id].num_of_cpus == 0)
            core_id = ( core_id + 1) % MAX_CPUS;
    }

    return core_id;
}

void free_mem( thread_info_t* tinfo, sys_info_t* sinfo)
{
    if ( tinfo) free( tinfo);

    if ( sinfo->cores) free( sinfo->cores);
}

#undef MAX_STR_LENGTH
#undef HANDLE_ERROR
#undef HANDLE_ERROR_EN
#undef FORWARD_ERROR
#undef FORWARD_ERROR_EN
#undef MIN
#undef MAX

