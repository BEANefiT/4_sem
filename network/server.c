#include "netlib.h"
#include "integrator.h"

#define HANDLE_ERROR( msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while ( 0)

#define HANDLE_ERROR_EN( msg, en) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while ( 0)

#define END_A 2.1
#define END_B 8.9

typedef struct
{
    int sockfd;
    double partition;
} client_t;

int      connect_clients();
int      wait_results();
int      init_jobs();

static int       nclients  = -1;
static in_port_t udp_port  = 0;
static in_port_t tcp_port  = 0;
static client_t* clients   = NULL;
static double    partition = 0.;
static double    result    = 0.;
static fd_set    readfds;
static fd_set    writefds;
static const int int_true  = 1;
static const int int_false = 0;

int main( int argc, char* argv[])
{
    if ( argc != 4)
    {
        if ( argc != 2)
        {
            fprintf( stderr, "Usage: %s nclients [udp_port] [tcp_port]\n",
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

    CHECK( ( nclients = str_2_uint( argv[1])));

    if ( nclients > MAX_CLIENTS_NUM)
    {
        char msg[64] = {};
        sprintf( msg, "Too many clients, support number <= %d\n",
                      MAX_CLIENTS_NUM);

        HANDLE_ERROR( msg);
    }

    if ( !( clients = ( client_t*)calloc( nclients, sizeof( *clients))))
        HANDLE_ERROR( "In calloc for clent_t* clients");

    if ( argc > 2)
    {
        int port = 0;
        
        CHECK( ( port = str_2_uint( argv[2])));
        udp_port = htons( port);

        CHECK( ( port = str_2_uint( argv[3])));
        tcp_port = htons( port);
    }

    CHECK( connect_clients());
    CHECK( init_jobs());
    CHECK( wait_results());

    printf( "Result is %lg\n", result);

    exit( EXIT_SUCCESS);
}

int connect_clients()
{
    int sockfd_listen = -1;
    
    CHECK_FORWARD( ( sockfd_listen = tcp_listen( tcp_port, MAX_CLIENTS_NUM)));
    CHECK_FORWARD( udp_broadcast_send( udp_port));

    #ifdef DEBUG
    printf( "UDP broadcast has been sent\n\n");
    #endif // DEBUG

    int sockfds[nclients];

    for ( int i = 0; i < nclients; i++)
        CHECK_FORWARD( ( clients[i].sockfd = tcp_accept( sockfd_listen)));

    return 0;
}

int wait_results()
{
    int ncycles = nclients;
    char* is_read = ( char*)calloc( nclients, sizeof( char));

    while ( 1)
    {
        FD_ZERO( &readfds);
        int nfds = 0;

        for ( int i = 0; i < nclients; i++)
        {
            if ( is_read[i])
                continue;

            if ( clients[i].sockfd >= nfds)
                nfds = clients[i].sockfd + 1;

            FD_SET( clients[i].sockfd, &readfds);
        }

        CHECK_FORWARD( select( nfds, &readfds, NULL, NULL, NULL));
        
        for ( int i = 0; i < nclients; i++)
        {
            if ( !FD_ISSET( clients[i].sockfd, &readfds))
                continue;

            is_read[i] = 1;
            ncycles --;

            double res = 0.;
            ssize_t read_res = -1;
            CHECK_FORWARD( ( read_res = read( clients[i].sockfd,
                                              &res, sizeof( double))));

            if ( !read_res)
            {
                char msg[64] = {};
                sprintf( msg, "Client #%d is dead\n", i + 1);
                FORWARD_ERROR_EN( msg, ECONNABORTED);
            }

            result += res;

            #ifdef DEBUG
            printf( "Result of Client #%d has been received\n\n", i + 1);
            #endif // DEBUG
        }

        if ( ncycles <= 0)
            break;
    }

    return 0;
}

int init_jobs()
{
    for ( int i = 0; i < nclients; i++)
    {
        ssize_t read_res;
        CHECK_FORWARD( ( read_res = read( clients[i].sockfd,
                                          &clients[i].partition,
                                          sizeof( double))));

        if ( !read_res)
        {
            char msg[64] = {};
            sprintf( msg, "Client #%d is dead\n", i + 1);
            FORWARD_ERROR_EN( msg, ECONNABORTED);
        }

        partition += clients[i].partition;
        
        #ifdef DEBUG
        printf( "Client #%d system information has been received\n"
                "\tsinfo.partition = %lg\n\n", i + 1, clients[i].partition);
        #endif // DEBUG
    }

    double end_a = END_A;
    double step  = ( END_B - END_A) / partition;

    for ( int i = 0; i < nclients; i++)
    {
        ends_t ends = {
            .end_a = end_a,
            .end_b = end_a + step * clients[i].partition
        };

        CHECK_FORWARD( write( clients[i].sockfd, &ends, sizeof( ends)));

        end_a += step * clients[i].partition;

        #ifdef DEBUG
        printf( "Calculation ends for Client #%d has been sent\n\n", i + 1);
        #endif // DEBUG
    }

    return 0;
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

