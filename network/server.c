#include "netlib.h"

int      connect_clients();
int      disconnect_clients();
int      init_jobs();
double   calculate();

static uint64_t  nclients = 0;
static in_port_t udp_port = 0;
static in_port_t tcp_port = 0;

int main( int argc, char* argv[])
{
    if ( argc != 4)
    {
        if ( argc != 2)
        {
            char msg[64] = {};

            sprintf( msg, "Usage: %s nclients [udp_port] [tcp_port]", argv[0]);

            HANDLE_ERROR_EN( msg, EINVAL);
        }

        else
        {
            printf( "Ports aren't identified, defaulting to:\n"
                    "\tudp_port = %" PRIu64 "\n"
                    "\ttcp_port = %" PRIu64 "\n",
                    DEFAULT_UDP_PORT, DEFAULT_TCP_PORT);

            udp_port = htons( DEFAULT_UDP_PORT);
            tcp_port = htons( DEFAULT_TCP_PORT);
        }
    }

    CHECK( ( nclients = str_2_uint( argv[1])));

    if ( argc > 2)
    {
        uint16_t port = 0;
        
        CHECK( ( port = str_2_uint( argv[2])));
        udp_port = htons( port);

        CHECK( ( port = str_2_uint( argv[3])));
        tcp_port = htons( port);
    }

    CHECK( connect_clients());

    CHECK( init_jobs());

    double result = 0.;

    CHECK( ( result = calculate()));

    CHECK( disconnect_clients());

    printf( "Result is %lg\n", result);

    exit( EXIT_SUCCESS);
}

int connect_clients()
{
    int udp_socket = -1;
        
    CHECK( ( udp_socket = socket( AF_INET, SOCK_DGRAM, 0)));

    char init_msg[8] = ".SERVER\0";

    struct sockaddr_in udp_dest_addr;
    memset( &udp_dest_addr, 0, sizeof( udp_dest_addr));

    udp_dest_addr.sin_family      = AF_INET;
    udp_dest_addr.sin_port        = udp_port;
    udp_dest_addr.sin_addr.s_addr = INADDR_ANY;

    CHECK( sendto( udp_socket, init_msg, 8, 0,
                   ( const struct sockaddr*)&udp_dest_addr,
                   ( socklen_t)sizeof( udp_dest_addr)));
}

