#ifndef __NETLIB_H__
#define __NETLIB_H__

#define DEBUG

#include "../scaler/scaler.h"

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define FORWARD_ERROR( msg) \
    do { int en = errno; fprintf( stderr, msg); errno = en; return -1; } while ( 0)

#define FORWARD_ERROR_EN( msg, en) \
    do { fprintf( stderr, msg); errno = en; return -1; } while ( 0)

#define CHECK( func) \
    do { if ( func == -1) HANDLE_ERROR( "In "#func); } while ( 0)

#define CHECK_FORWARD( func) \
    do { if ( func == -1) FORWARD_ERROR( "In "#func"\n"); } while ( 0)

#define MIN( a, b) ( a > b ? b : a)

#define MAX( a, b) ( a > b ? a : b)

#define DEFAULT_UDP_PORT 48655
#define DEFAULT_TCP_PORT 48656
#define SIGN ".SERVER\0"
#define SIGN_LEN 8
#define MAX_CLIENTS_NUM 64

struct sockaddr_in udp_broadcast_recv( const in_port_t);
ssize_t            udp_broadcast_send( const in_port_t);
int                tcp_handshake_accept( const in_port_t, int* sockfds,
                                         int nclients, int backlog);
int                tcp_handshake_connect( const in_port_t, struct sockaddr_in*);

#endif // __NETLIB_H__

