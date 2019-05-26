#include "netlib.h"

int      connect_server();
int      disconnect_server( double result);
int      init_threads();
double   calculate();

static int       nthreads = 0;
static in_port_t tcp_port = 0;
static in_port_t udp_port = 0;

int main( int argc, char* argv[])
{
    if ( argc != 4)
    {
        if ( argc != 2)
        {
            char msg[64] = {};
            sprintf( msg, "Usage: %s nthreads [udp_port] [tcp_port]",
                     argv[0]);

            HANDLE_ERROR_EN( msg, EINVAL);
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

    CHECK( ( nthreads = str_2_uint( argv[1])));

    if ( argc > 2)
    {
        int port = 0;
    
        CHECK( ( port = str_2_uint( argv[2])));
        udp_port = htons( port);

        CHECK( ( port = str_2_uint( argv[3])));
        tcp_port = htons( port);
    }

    CHECK( connect_server());

    //CHECK( init_threads());

    double result = 0.;

    //CHECK( ( result = calculate()));

    //CHECK( disconnect_server( result));

    exit( EXIT_SUCCESS);
}

int connect_server()
{
    CHECK_FORWARD( udp_broadcast_recv( udp_port));

    #ifdef DEBUG
    printf( "UDP broadcast has been received\n\n");
    #endif // DEBUG
}

