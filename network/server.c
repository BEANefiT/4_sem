#define DEBUG

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define HANDLE_ERROR( msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while ( 0)

#define HANDLE_ERROR_EN( msg, en) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while ( 0)

#define FORWARD_ERROR( msg) \
    do { int en = errno; fprintf( stderr, msg); errno = en; return -1; } while ( 0)

#define FORWARD_ERROR_EN( msg, en) \
    do { fprintf( stderr, msg); errno = en; return -1; } while ( 0)

#define CHECK( func) \
    do { if ( func == -1) HANDLE_ERROR( "In"#func); } while ( 0)


#define MIN( a, b) ( a > b ? b : a)

#define MAX( a, b) ( a > b ? a : b)

uint64_t str_2_uint( char*);

uint64_t  nclients = 0;
in_port_t tcp_port = 0;
in_port_t udp_port = 0;

int main( int argc, char* argv[])
{
    if ( argc != 4)
        HANDLE_ERROR_EN( "Invalid number of args, expected - 3: "
                         "nclients, udp_port, tcp_port\n", EINVAL);

    CHECK( ( nclients = str_2_uint( argv[1])));

    uint16_t port = 0;
    
    CHECK( ( port = str_2_uint( argv[2])));
    udp_port = htons( port);

    CHECK( ( port = str_2_uint( argv[3])));
    tcp_port = htons( port);

    CHECK( connect_clients());

    CHECK( init_jobs());

    double result = 0.;

    CHECK( ( result = calculate()));

    CHECK( disconnect_clients());

    printf( "Result is %lg\n", result);

    exit( EXIT_SUCCESS);
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
