#include "netlib.h"

#define HANDLE_ERROR( msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while ( 0)

#define HANDLE_ERROR_EN( msg, en) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while ( 0)

static int int_true = 1;
static int int_false = 0;

ssize_t udp_broadcast_send( const in_port_t port)
{
    int sockfd = -1;
        
    CHECK_FORWARD( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0)));

    CHECK_FORWARD( setsockopt( sockfd, SOL_SOCKET, SO_BROADCAST,
                               &int_true, sizeof( int)));

    char init_msg[SIGN_LEN] = SIGN;

    struct sockaddr_in dest_addr;
    memset( &dest_addr, 0, sizeof( dest_addr));

    dest_addr.sin_family      = AF_INET;
    dest_addr.sin_port        = port;
    dest_addr.sin_addr.s_addr = htonl( INADDR_BROADCAST);

    ssize_t send_res = sendto( sockfd, init_msg, SIGN_LEN, 0,
                               ( const struct sockaddr*)&dest_addr,
                               ( socklen_t)sizeof( dest_addr));

    CHECK_FORWARD( send_res);
    
    return send_res;
}

struct sockaddr_in udp_broadcast_recv( const in_port_t port)
{
    int sockfd = -1;
        
    CHECK( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0)));

    CHECK( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,
                       &int_true, sizeof( int)));

    struct sockaddr_in src_addr;
    memset( &src_addr, 0, sizeof( src_addr));
    socklen_t addrlen = sizeof( src_addr);

    src_addr.sin_family      = AF_INET;
    src_addr.sin_port        = port;
    src_addr.sin_addr.s_addr = htonl( INADDR_ANY);

    CHECK( bind( sockfd, ( const struct sockaddr*)&src_addr, addrlen));

    ssize_t recv_res = -1;

    while( 1)
    {
        char init_msg[SIGN_LEN];

        recv_res = recvfrom( sockfd, init_msg, SIGN_LEN, 0,
                             ( struct sockaddr*)&src_addr, &addrlen);

        CHECK( recv_res);

        if ( !strcmp( init_msg, SIGN) && src_addr.sin_port == port)
            break;

        #ifdef DEBUG
        printf( "Received wrong message\n"
                "\tinit_msg = %s;\tsrc_addr.sin_port = %d\n\n",
                init_msg, src_addr.sin_port);
        #endif //DEBUG
    }
    
    return src_addr;
}

int tcp_listen( const in_port_t port, int backlog)
{
    int sockfd_listen = -1;

    CHECK_FORWARD( ( sockfd_listen = socket( AF_INET, SOCK_STREAM, 0)));
    CHECK_FORWARD( ( setsockopt( sockfd_listen, SOL_SOCKET, SO_REUSEADDR,
                                 &int_true, sizeof( int))));

    struct sockaddr_in addr;
    memset( &addr, 0, sizeof( addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = port;
    addr.sin_addr.s_addr = htonl( INADDR_ANY);

    CHECK_FORWARD( bind( sockfd_listen, ( const struct sockaddr*)&addr,
                         ( socklen_t)sizeof( addr)));
    CHECK_FORWARD( listen( sockfd_listen, backlog));

    return sockfd_listen;
}

int tcp_connect( const in_port_t port, struct sockaddr_in* dest_addr)
{
    int sockfd = -1;

    CHECK_FORWARD( ( sockfd = socket( AF_INET, SOCK_STREAM, 0)));
    CHECK_FORWARD( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,
                               &int_true, sizeof( int)));

    dest_addr -> sin_port = port;
    CHECK_FORWARD( connect( sockfd, ( const struct sockaddr*)dest_addr,
                            ( socklen_t)sizeof( *dest_addr)));

    CHECK_FORWARD( setsockopt( sockfd, SOL_SOCKET, SO_KEEPALIVE,
                               &int_true, sizeof( int)));

    CHECK_FORWARD( setsockopt( sockfd, IPPROTO_TCP, TCP_KEEPIDLE,
                               &int_true, sizeof( int)));
    CHECK_FORWARD( setsockopt( sockfd, IPPROTO_TCP, TCP_KEEPINTVL,
                               &int_true, sizeof( int)));
    CHECK_FORWARD( setsockopt( sockfd, IPPROTO_TCP, TCP_KEEPCNT,
                               &int_true, sizeof( int)));

    CHECK_FORWARD( fcntl( sockfd, F_SETOWN, getpid()));


    return sockfd;
}

int tcp_accept( int sockfd_listen)
{
    struct sockaddr_in addr;
    memset( &addr, 0, sizeof( addr));
   
    socklen_t addrlen = sizeof( addr);

    int sockfd_accept = -1;
    CHECK_FORWARD( ( sockfd_accept = accept( sockfd_listen,
                                             ( struct sockaddr*)&addr,
                                             &addrlen)));

    CHECK_FORWARD( setsockopt( sockfd_accept, SOL_SOCKET, SO_KEEPALIVE,
                               &int_true, sizeof( int)));

    CHECK_FORWARD( setsockopt( sockfd_accept, IPPROTO_TCP, TCP_KEEPIDLE,
                               &int_true, sizeof( int)));
    CHECK_FORWARD( setsockopt( sockfd_accept, IPPROTO_TCP, TCP_KEEPINTVL,
                               &int_true, sizeof( int)));
    CHECK_FORWARD( setsockopt( sockfd_accept, IPPROTO_TCP, TCP_KEEPCNT,
                               &int_true, sizeof( int)));

    CHECK_FORWARD( fcntl( sockfd_accept, F_SETOWN, getpid()));

    #ifdef DEBUG
    printf( "Accepted TCP socket = %d\n\n", sockfd_accept);
    #endif // DEBUG

    return sockfd_accept;
}

#undef HANDLE_ERROR
#undef HANDLE_ERROR_EN

