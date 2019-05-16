#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

double calculate( double a, double b);

int main( int argc, char* argv[])
{
    double result = calculate( 1., 2.);

    if ( errno)
    {
        perror( NULL);
        exit( EXIT_FAILURE);
    }

    printf( "Result is %lg\n", result);

    exit( EXIT_SUCCESS);
}


/** Simple function which can integrate only one set up function;
 *  calculation is made from ( x = a) to ( x = b). */
double calculate( double a, double b)
{
    double dx = 1e-9;
    double result = 0;

    for ( double x = a; x <= b; x += dx)
    {
        result += ( 1 / x) * ( 1 / ( x - 0.5)) * dx;

        if ( errno)
            return 0;
    }

    return result;
}
