#include "netlib.h"

ssize_t udp_broadcast_send( in_port_t port)
{
    int sockfd = -1;
        
    CHECK_FORWARD( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0)));

    char init_msg[SIGN_LEN] = SIGN;

    struct sockaddr_in dest_addr;
    memset( &dest_addr, 0, sizeof( dest_addr));

    dest_addr.sin_family      = AF_INET;
    dest_addr.sin_port        = port;
    dest_addr.sin_addr.s_addr = INADDR_ANY;

    ssize_t send_res = sendto( sockfd, init_msg, SIGN_LEN, 0,
                               ( const struct sockaddr*)&dest_addr,
                               ( socklen_t)sizeof( dest_addr));

    CHECK_FORWARD( send_res);
    
    return send_res;
}

ssize_t udp_broadcast_recv( in_port_t port)
{
    int sockfd = -1;
        
    CHECK_FORWARD( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0)));

    ssize_t recv_res = -1;

    while( 1)
    {
        struct sockaddr_in src_addr;
        memset( &src_addr, 0, sizeof( src_addr));

        socklen_t addrlen = sizeof( src_addr);

        char init_msg[SIGN_LEN];

        recv_res = recvfrom( sockfd, init_msg, SIGN_LEN, 0,
                             ( struct sockaddr*)&src_addr, addrlen);

        CHECK_FORWARD( recv_res);

        if ( !strcmp( init_msg, SIGN) && src_addr.sin_port == port)
            break;
    }
    
    return recv_res;
}

int str_2_uint( char* str)
{
    char* endptr = NULL;

    int n = strtoul( str, &endptr, 10);

    if ( str == NULL || *endptr != '\0')
        FORWARD_ERROR_EN( "In strtol()\n", EINVAL);

    else if (errno)
        FORWARD_ERROR( "In strtol()\n");

    return n;
}

