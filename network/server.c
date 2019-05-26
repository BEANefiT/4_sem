#include "netlib.h"

typedef struct
{
    int socket;
    double partition;
} client_t;

int      connect_clients();
int      disconnect_clients();
int      init_jobs();
double   calculate();

static int       nclients = 0;
static in_port_t udp_port = -1;
static in_port_t tcp_port = -1;
static client_t* clients  = NULL;

int main( int argc, char* argv[])
{
    if ( argc != 4)
    {
        if ( argc != 2)
        {
            char msg[64] = {};
            sprintf( msg, "Usage: %s nclients [udp_port] [tcp_port]",
                     argv[0]);

            HANDLE_ERROR_EN( msg, EINVAL);
        }

        else
        {
            printf( "Ports aren't identified, defaulting to:\n"
                    "\tudp_port = %d\n"
                    "\ttcp_port = %d\n",
                    DEFAULT_UDP_PORT, DEFAULT_TCP_PORT);

            udp_port = htons( DEFAULT_UDP_PORT);
            tcp_port = htons( DEFAULT_TCP_PORT);
        }
    }

    CHECK( ( nclients = str_2_uint( argv[1])));

    if ( ( clients = ( client_t*)calloc( nclients, sizeof( *clients))))
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

    double result = 0.;

    CHECK( ( result = calculate()));

    CHECK( disconnect_clients());

    printf( "Result is %lg\n", result);

    exit( EXIT_SUCCESS);
}

int connect_clients()
{
    CHECK_FORWARD( udp_broadcast_send( udp_port));

    for ( int i = 0; i < nclients; i++)
        CHECK_FORWARD( tcp_handshake( tcp_port, &clients[i].socket));
}

