#include "scaler.h"

#define MAX_STR_LENGTH 128

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

    sinfo->cores = ( core_info_t*)calloc( MAX_CPUS, sizeof( *sinfo->cores));

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

/** Initialise threads' partitions;
 *  Divide threads by Cores and CPUs
 */
int init_tinfo( int nthreads, thread_info_t** tinfo, sys_info_t* sinfo)
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

    for ( int i = 0; i < sinfo->num_of_cores; i++)
    {
        core_info_t* core = &sinfo->cores[core_id];
        core->nthreads = nthreads / sinfo->num_of_cores
                         + ( i < ( nthreads % sinfo->num_of_cores));

        if ( core->ncpus == 2 && num_of_used_double_cores)
        {
            num_of_used_double_cores --;
            core->uses_ht = 1;
            core->partition = HT_SPEEDUP;
            core->cpus[0].partition = HT_SPEEDUP / 2;
            core->cpus[1].partition = HT_SPEEDUP / 2;
            core->cpus[0].nthreads  = core->nthreads / 2 + core->nthreads % 2;
            core->cpus[1].nthreads  = core->nthreads / 2;
        }

        else
        {
            core->partition = 1.;
            core->cpus[0].partition = 1.;
            core->cpus[0].nthreads  = core->nthreads;
        }

        core_id = get_core_id_by_prev( core_id, sinfo);
    }

    core_id = get_core_id( 0, sinfo);

    for ( int i = 0; i < MAX( nthreads, sinfo->num_of_cores); i++)
    {
        thread_info_t* thread = tinfo[i];
        thread->tnum    = i + 1;
        thread->core_id = core_id;

        core_info_t* core = &sinfo->cores[core_id];
        cpu_info_t*  cpu = NULL;

        if ( ( ( i / sinfo->num_of_cores) % 2) && core->uses_ht)
            cpu = &core->cpus[1];

        else
            cpu = &core->cpus[0];

        thread->cpu_id = cpu->cpu_id;

        if ( cpu->nthreads)
            thread->partition = cpu->partition / cpu->nthreads;

        else
            thread->partition = 0.;

        core_id = get_core_id_by_prev( core_id, sinfo);
    }

    return 0;
}

int str_2_uint( char* str)
{
    char* endptr = NULL;

    int n = strtol( str, &endptr, 10);

    if ( str == NULL || *endptr != '\0')
        FORWARD_ERROR_EN( "In strtol()\n", EINVAL);

    else if (errno)
        FORWARD_ERROR( "In strtol()\n");

    return n;
}

int get_core_id_by_prev( int core_id_prev, sys_info_t* sinfo)
{
    int core_id = ( core_id_prev + 1) % MAX_CPUS;

    while ( sinfo->cores[core_id].ncpus == 0)
        core_id = ( core_id + 1) % MAX_CPUS;

    return core_id;
}

int get_core_id( int core_num, sys_info_t* sinfo)
{
    int core_id = 0;

    while ( sinfo->cores[core_id].ncpus == 0)
        core_id = ( core_id + 1) % MAX_CPUS;

    for ( int i = 0; i < core_num; i++)
    {
        core_id = ( core_id + 1) % MAX_CPUS;

        while ( sinfo->cores[core_id].ncpus == 0)
            core_id = ( core_id + 1) % MAX_CPUS;
    }

    return core_id;
}

#undef MAX_STR_LENGTH
#undef HANDLE_ERROR
#undef HANDLE_ERROR_EN
#undef FORWARD_ERROR
#undef FORWARD_ERROR_EN
#undef MIN
#undef MAX

#undef _GNU_SOURCE
#undef MAX_CPUS
#undef MAX_THREADS_NUM
#undef HT_SPEEDUP

