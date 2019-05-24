#include "netlib.h"

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

