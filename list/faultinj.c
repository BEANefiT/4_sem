#include "faultinj.h"

void *faultcalloc (size_t nmemb, size_t size)
{
    if ((rand() % 100) > 30)
        return calloc (nmemb, size);

    else 
    {
        errno = ENOMEM;
        return NULL;
    }
}
