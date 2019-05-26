#include "netlib.h"

int udp_broadcast( in_port_t udp_port)
{
    int udp_socket = -1;
        
    CHECK_FORWARD( ( udp_socket = socket( AF_INET, SOCK_DGRAM, 0)));

    char init_msg[8] = ".SERVER\0";

    struct sockaddr_in udp_dest_addr;
    memset( &udp_dest_addr, 0, sizeof( udp_dest_addr));

    udp_dest_addr.sin_family      = AF_INET;
    udp_dest_addr.sin_port        = udp_port;
    udp_dest_addr.sin_addr.s_addr = INADDR_ANY;

    CHECK_FORWARD( sendto( udp_socket, init_msg, 8, 0,
                           ( const struct sockaddr*)&udp_dest_addr,
                           ( socklen_t)sizeof( udp_dest_addr)));
}

uint64_t str_2_uint( char* str)
{
    char* endptr = NULL;

    uint64_t n = strtoul( str, &endptr, 10);

    if ( str == NULL || *endptr != '\0')
        FORWARD_ERROR_EN( "In strtol()\n", EINVAL);

    else if (errno)
        FORWARD_ERROR( "In strtol()\n");

    return n;
}

