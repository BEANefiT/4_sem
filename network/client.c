#include "netlib.h"
#include "integrator.h"

#include <signal.h>

#define FUNC( x) ( ( 0.23 * x * x) - ( x - 0.5))

#define HANDLE_ERROR( msg)                                                \
    do {                                                                  \
        perror(msg);                                                      \
                                                                          \
        if ( tinfo)                                                       \
        {                                                                 \
            for ( int i = 0; i < MAX( nthreads, sinfo.num_of_cores); i++) \
                if ( tinfo[i])                                            \
                    free( tinfo[i]);                                      \
                                                                          \
            free( tinfo);                                                 \
        }                                                                 \
                                                                          \
        if ( sinfo.cores) free( sinfo.cores);                             \
                                                                          \
        exit(EXIT_FAILURE);                                               \
    } while ( 0)

#define HANDLE_ERROR_EN( msg, en)                                         \
    do {                                                                  \
        errno = en;                                                       \
        perror(msg);                                                      \
                                                                          \
        if ( tinfo)                                                       \
        {                                                                 \
            for ( int i = 0; i < MAX( nthreads, sinfo.num_of_cores); i++) \
                if ( tinfo[i])                                            \
                    free( tinfo[i]);                                      \
                                                                          \
            free( tinfo);                                                 \
        }                                                                 \
                                                                          \
        if ( sinfo.cores) free( sinfo.cores);                             \
                                                                          \
        exit(EXIT_FAILURE);                                               \
    } while ( 0)

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

int          connect_server();
int          disconnect_server();
int          init_ends();
int          create_threads();
int          join_threads();
void         sigio_handler( int);
static void* calculate( void* arg);

static int               nthreads   = -1;
static in_port_t         tcp_port   = 0;
static in_port_t         udp_port   = 0;
static int               tcp_sockfd = -1;
static double            result     = 0.;
static sys_info_t        sinfo      = {0, 0, 0, 0, 0, 0., NULL};
static thread_module_t** tinfo      = NULL;
static ends_t            ends       = {0, 0};
static const int         int_true   = 1;
static const int         int_false  = 0;

int main( int argc, char* argv[])
{
    if ( argc != 4)
    {
        if ( argc != 2)
        {
            fprintf( stderr, "Usage: %s nthreads [udp_port] [tcp_port]\n",
                     argv[0]);

            exit( EXIT_FAILURE);
        }

        else
        {
            printf( "Ports aren't identified, defaulting to:\n"
                    "\tudp_port = %d\n"
                    "\ttcp_port = %d\n\n",
                    DEFAULT_UDP_PORT, DEFAULT_TCP_PORT);

            udp_port = htons( DEFAULT_UDP_PORT);
            tcp_port = htons( DEFAULT_TCP_PORT);
        }
    }

    struct sigaction act = {
            .sa_handler = sigio_handler
    };

    CHECK( sigaction( SIGIO, &act, NULL));

    CHECK( init_sysinfo( &sinfo));
    CHECK( ( nthreads = str_2_uint( argv[1])));

    tinfo = ( thread_module_t**)calloc( MAX( nthreads, sinfo.num_of_cores),
                                        sizeof( *tinfo));

    if ( tinfo == NULL)
        HANDLE_ERROR( "In calloc of tinfo");

    for ( int i = 0; i < MAX( nthreads, sinfo.num_of_cores); i++)
    {
        tinfo[i] = ( thread_module_t*)calloc( 1, sizeof( *tinfo[i]));

        if ( tinfo[i] == NULL)
            HANDLE_ERROR( "In calloc of tinfo[i]");
    }

    CHECK( init_tinfo( nthreads, ( thread_info_t**)tinfo, &sinfo));

    if ( argc > 2)
    {
        int port = 0;
    
        CHECK( ( port = str_2_uint( argv[2])));
        udp_port = htons( port);

        CHECK( ( port = str_2_uint( argv[3])));
        tcp_port = htons( port);
    }

    CHECK( connect_server());
    CHECK( init_ends());
    CHECK( fcntl( tcp_sockfd, F_SETFL, O_ASYNC));
    CHECK( create_threads());
    CHECK( join_threads());
    CHECK( fcntl( tcp_sockfd, F_SETFL, 0));
    CHECK( disconnect_server());
    exit( EXIT_SUCCESS);
}

int connect_server()
{
    struct sockaddr_in server_addr = udp_broadcast_recv( udp_port);

    #ifdef DEBUG
    printf( "UDP broadcast has been received\n\n");
    #endif // DEBUG

    CHECK_FORWARD( ( tcp_sockfd = tcp_connect( tcp_port, &server_addr)));

    #ifdef DEBUG
    printf( "TCP-handshake has been sent\n\n");
    #endif //DEBUG

    return 0;
}

int disconnect_server()
{
    CHECK_FORWARD( write( tcp_sockfd, &result, sizeof( double)));

    #ifdef DEBUG
    printf( "Result has been sent to server\n\n");
    #endif // DEBUG
}

int init_ends()
{
    CHECK_FORWARD( write( tcp_sockfd, &sinfo.partition, sizeof( double)));

    #ifdef DEBUG
    printf( "Client system information has been sent\n"
            "\tsinfo.partition = %lg\n\n", sinfo.partition);
    #endif // DEBUG

    fd_set readfd;
    FD_ZERO( &readfd);
    FD_SET( tcp_sockfd, &readfd);
    select( tcp_sockfd + 1, &readfd, NULL, NULL, &(( struct timeval){2, 0}));

    if ( !FD_ISSET( tcp_sockfd, &readfd))
        FORWARD_ERROR_EN( "Too many clients\n", ECONNREFUSED);

    CHECK_FORWARD( read( tcp_sockfd, &ends, sizeof( ends)));

    #ifdef DEBUG
    printf( "Received ends of calculating\n"
            "\tend_a = %lg    end_b = %lg\n\n", ends.end_a, ends.end_b);
    #endif // DEBUG
    
    return 0;
}

int create_threads()
{
    pthread_attr_t attr;
    cpu_set_t      cpuset;

    if ( pthread_attr_init( &attr) != 0)
        FORWARD_ERROR( "In pthread_attr_init()\n");

    double end_a = ends.end_a;
    double step = ( ends.end_b - ends.end_a) / sinfo.partition;

    for ( int tnum = 0; tnum < MAX( nthreads, sinfo.num_of_cores); tnum++)
    {
        thread_module_t* thread = tinfo[tnum];
        thread->ends.end_a = end_a;
        thread->ends.end_b = end_a;

        if ( thread->partition != 0.)
        {
            end_a += step * thread->partition;
            thread->ends.end_b = end_a;
        }

        else
        {
            thread->ends.end_a = ends.end_a;
            thread->ends.end_b = ends.end_b;
        }

        #ifdef DEBUG
        printf( "Thread #%d\n"
                "\tcore_id = %d\n"
                "\tcpu_id = %d\n"
                "\tpartition = %lg\n"
                "\tend_a = %lg\tend_b = %lg\n\n", tnum + 1,
                thread->core_id, thread->cpu_id, thread->partition,
                thread->ends.end_a, thread->ends.end_b);
        #endif // DEBUG
        
        CPU_ZERO( &cpuset);
        CPU_SET( thread->cpu_id, &cpuset);

        if ( pthread_attr_setaffinity_np( &attr, sizeof( cpu_set_t),
                                          &cpuset) != 0)
            FORWARD_ERROR( "In pthread_attr_setaffinity_np()\n");

        if ( pthread_create( &( thread->tid), &attr, &calculate, thread) != 0)
            FORWARD_ERROR( "In pthread_create\n");
    }

    if ( pthread_attr_destroy( &attr) != 0)
        FORWARD_ERROR( "In pthread_attr_destroy()\n");

    return 0;
}

int join_threads()
{
    for ( int tnum = 0; tnum < nthreads; tnum++)
    {
        if ( pthread_join( tinfo[tnum]->tid, NULL) != 0)
            FORWARD_ERROR( "In pthread_join\n");

        result += tinfo[tnum]->result;
    }
    
    #ifdef DEBUG
    printf( "Calculation has been completed\n\n");
    #endif // DEBUG

    return 0;
}

static void* calculate( void* arg)
{
    thread_module_t* info = arg;

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

void sigio_handler( int arg)
{
    HANDLE_ERROR_EN( "Server is dead", ECONNABORTED);
}

#undef HANDLE_ERROR
#undef HANDLE_ERROR_EN
#undef FORWARD_ERROR
#undef FORWARD_ERROR_EN
#undef CHECK
#undef CHECK_FORWARD
#undef MIN
#undef MAX
#undef DEFAULT_UDP_PORT
#undef DEFAULT_TCP_PORT
#undef SIGN
#undef SIGN_LEN
#undef MAX_CLIENTS_NUM

