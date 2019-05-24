#include "netlib.h"

int      connect_server();
int      disconnect_server( double result);
int      init_threads();
double   calculate();

static uint64_t  nthreads = 0;
static in_port_t tcp_port = 0;
static in_port_t udp_port = 0;

int main( int argc, char* argv[])
{
    if ( argc != 4)
        HANDLE_ERROR_EN( "Invalid number of args, expected - 3: "
                         "nthreads, udp_port, tcp_port\n", EINVAL);

    CHECK( ( nthreads = str_2_uint( argv[1])));

    uint16_t port = 0;
    
    CHECK( ( port = str_2_uint( argv[2])));
    udp_port = htons( port);

    CHECK( ( port = str_2_uint( argv[3])));
    tcp_port = htons( port);

    CHECK( connect_server());

    CHECK( init_threads());

    double result = 0.;

    CHECK( ( result = calculate()));

    CHECK( disconnect_server( result));

    exit( EXIT_SUCCESS);
}

