#include <stdio.h>
#include "list.h"

#define MSG_SIZE 64
#define ARR_SIZE 1e5

list_t  *list       = NULL;
list_t  *fake_list  = NULL;
char    *array1     = NULL;
char    *array2     = NULL;
FILE    *flog       = NULL;
FILE    *out        = NULL;

void error (const char *s)
{
    if (errno)
    {
        perror (s);
        errno = 0;
    }

    else
        fprintf (stderr, s);
}

void fail (const char *s)
{
    fprintf (flog, s);

    list_erase  (list);
    list_erase  (fake_list);
    free        (list);
    free        (fake_list);
    free        (array1);
    free        (array2);
    fclose      (flog);
    fclose      (out);

    exit (EXIT_FAILURE);
}

void erase()
{
    if (list_erase (list) == -1)
        fail ("Error while list_erase\n");

    if (list_erase (fake_list) == -1)
        fail ("Error while fake_list_erase\n");
        
    for (int i = 0; i < ARR_SIZE; i++)
    {
        array1[i] = 0;
        array2[i] = 0;
    }
}

int main()
{
    flog = fopen ("log", "w");
    out = fopen ("out", "w");

    while ((list = list_constr()) == NULL)
        ;

    while ((fake_list = list_constr()) == NULL)
        ;

    array1 = (char *) calloc(ARR_SIZE, 1);
    array2 = (char *) calloc(ARR_SIZE, 1);

    for (int i = 0; i < 99; i++)
    {
        if (list_push_tail (list, i) == -1)
        {
            array1[i] = 1;

            char s [MSG_SIZE];
            sprintf (s, "ERROR while list_push_tail (%p, %d)\n", list, i);
            error (s);
        }
    }

    int list_print (int val, void* file)
    {
        int res = fprintf (file, "%d\t", val);

        fflush (file);

        array2[val] = 1;

        return res;
    }

    if (list_process (list, &list_print, out) == -1)
    {
        error ("Error while list_process\n");
    }

    fprintf (out, "\n");

    for (int i = 0; i < 99; i++)
    {
        if (array1[i] == array2[i])
        {
            char msg[MSG_SIZE] = {};
            sprintf (msg, "FAULT array1[%d] == array2[%d]\n", i, i);
            fail (msg);
        }
    }

    erase();

    for (int i = 0; i < 99; i++)
    {
        if (list_push_head (list, 98 - i) == -1)
        {
            array1[98 - i] = 1;

            char msg [MSG_SIZE];
            sprintf (msg, "ERROR in list_push_head(%p, %d)\n", list, i);
            error (msg);
        }
    }

    if (list_process (list, &list_print, out) == -1)
    {
        error ("Error while list_process\n");
    }

    fprintf (out, "\n");

    for (int i = 0; i < 99; i++)
    {
        if (array1[i] == array2[i])
        {
            char msg[MSG_SIZE] = {};
            sprintf (msg, "FAULT array1[%d] == array2[%d]\n", i, i);
            fail (msg);
        }
    }

    while (list_push_head (fake_list, 123) == -1)
        ;

    if (list_insert (list, list_get_tail(fake_list), 797797) != -1)
    {
        fail ("list_insert with fake_node\n");
    }

    if (list_del (list, list_get_tail (fake_list)) != -1)
    {
        fail ("list_del with fake_node\n");
    }

    erase();

    for (int i = 0; i < 11; i++)
    {
        if (i != 0)
            i += 11999;

        if (list_insert (list, list_get_tail (list), i) == -1)
        {
            array1[i] = 1;

            char msg [MSG_SIZE];
            sprintf (msg, "ERROR while list_insert (%p, %d)\n", list, i);
            error (msg);
        }

        if (i != 0)
            i -= 11999;
    }

    for (int i = 1; i < 12000; i++)
    {
        if (list_insert (list, list_get_head (list), i) == -1)
        {
            array1[i] = 1;

            char msg [MSG_SIZE];
            sprintf (msg, "ERROR while list_insert (%p, %d)\n", list, i);
            error (msg);
        }
    }

    if (list_process (list, &list_print, out) == -1)
    {
        error ("Error while list_process\n");
    }

    fprintf (out, "\n");

    for (int i = 0; i < 12010; i++)
    {
        if (array1[i] == array2[i])
        {
            char msg[MSG_SIZE] = {};
            sprintf (msg, "FAULT array1[%d] == array2[%d]\n", i, i);
            fail (msg);
        }
    }

    if (list_del (list, list_get(list, 1000)) == -1)
    {
        fail ("ERROR while list_del (list, list_get(1000))\n");
    }

    if (list_del (list, list_get_tail(list)) == -1)
    {
        fail ("ERROR while list_del(list, tail)\n");
    }

    if (list_get(list, 9999999999999) != NULL)
    {
        fail ("ERROR while get(pos > size)\n");
    }

    fprintf (out, "elem ptr = %p\n", list_find (list, 1005));

    int list_error (int val, void* buf)
    {
        return -1;
    }

    if (list_process (list, &list_error, NULL) != -1)
    {
        fail ("ERROR in list_error()\n");
    }

    list_erase  (list);
    list_erase  (fake_list);
    free        (list);
    free        (fake_list);
    free        (array1);
    free        (array2);
    fclose      (flog);
    fclose      (out);

    exit (EXIT_SUCCESS);
}
