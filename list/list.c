#include "list.h"

#define CHECK_PTR( ptr, ret_val )   \
do                                  \
{                                   \
    if (! ptr)                      \
    {                               \
        errno = EINVAL;             \
                                    \
        return ret_val;             \
    }                               \
}while (0)

struct list_elem
{
    data_t      value;
    list_elem*  next;
};

struct list_t
{
    list_elem*  head;
    list_elem*  tail;
    size_t      size;
};

/*
    returns pointer to list (may be NULL)
*/
list_t* list_constr()
{
    list_t* list = (list_t *) calloc (sizeof (list_t), 1);

    if (list)
    {
        list -> head = NULL;
        list -> tail = NULL;
        list -> size = 0;
    }

    return list;
}

/*
    returns -1 if error
    returns 0  if success
*/
int list_push_tail (list_t* list, data_t val)
{
    CHECK_PTR (list, -1);

    list_elem* elem = (list_elem *) calloc (sizeof (list_elem), 1);

    if (!elem)
        return -1;

    elem -> value = val;
    elem -> next  = NULL;

    if (!(list -> size))
    {
        list -> head = elem;
        list -> tail = elem;
        list -> size++;

        return 0;
    }

    if (!(list -> tail))
    {
        errno = EINVAL;

        return -1;
    }

    list -> tail -> next = elem;
    list -> tail         = elem;
    list -> size++;

    return 0;
}

/*
    returns -1 if error
    returns 0  if success
*/
int list_push_head (list_t* list, data_t val)
{
    CHECK_PTR (list, -1);

    list_elem* elem = (list_elem *) calloc (sizeof (list_elem), 1);

    if (!elem)
        return -1;

    elem -> value = val;
    elem -> next  = list -> head;
    list -> head = elem;

    if (!(list -> size))
        list -> tail = elem;

    list -> size++;
    
    return 0;
}

/*
    returns -1 if error
    returns 0  if success
*/
int list_insert (list_t* list, list_elem* pos, data_t val)
{
    CHECK_PTR (list, -1);
    CHECK_PTR (pos,  -1);

    if ((list_check_elem (list, pos)) != 1)
    {
        errno = EINVAL;

        return -1;
    }

    if (pos == list -> tail)
        return list_push_tail (list, val);

    list_elem* elem = (list_elem *) calloc (sizeof (list_elem), 1);

    if (!elem)
        return -1;

    elem -> value = val;
    elem -> next  = pos -> next;
    pos  -> next  = elem;
    
    list -> size++;

    return 0;
}

/*
    returns 1  if elem is in list
    returns 0  if elem is not in list
    returns -1 if error
*/
int list_check_elem (list_t* list, list_elem* elem)
{
    CHECK_PTR (list, -1);
    CHECK_PTR (elem, -1);

    list_elem* tmp = list -> head;
    
    for (size_t i = 0; i < list -> size; i++)
    {
        CHECK_PTR (tmp, -1);

        if (tmp != elem)
            tmp = tmp -> next;

        else
            return 1;
    }

    return 0;
}

/*
    returns -1 if error
    returns 0  if success
*/
int list_del (list_t* list, list_elem* elem)
{
    if ((list_check_elem (list, elem)) != 1)
    {
        errno = EINVAL;

        return -1;
    }

    if ((list -> size) == 1)
    {
        list -> head = NULL;
        list -> tail = NULL;
        free (elem);
        list -> size--;

        return 0;
    }

    if (elem == (list -> head))
    {
        list -> head = elem -> next;
        free (elem);
        list -> size--;

        return 0;
    }

    list_elem* prev = list -> head;

    for (size_t i = 0; i < (list -> size + 1); i++)
    {
        CHECK_PTR (prev, -1);

        if ((prev -> next) != elem)
            prev = prev -> next;

        else
            break;
    }

    prev -> next = elem -> next;

    if (elem == (list -> tail))
        list -> tail = prev;

    free (elem);
    list -> size--;

    return 0;
}

int list_erase (list_t* list)
{
    CHECK_PTR (list, -1);

    list_elem* elem = list -> head;

    size_t size = list -> size;
    for (size_t i = 0; i < size; i++)
    {
        CHECK_PTR (elem, -1);

        list_del (list, elem);

        elem = elem -> next;
    }
}

list_elem* list_get_head (list_t* list)
{
    CHECK_PTR (list, NULL);

    return list -> head;
}

list_elem* list_get_tail (list_t* list)
{
    CHECK_PTR (list, NULL);

    return list -> tail;
}

list_elem* list_get (list_t* list, size_t pos)
{
    CHECK_PTR (list, NULL);
    
    if (pos >= (list -> size))
    {
        errno = EINVAL;

        return NULL;
    }

    list_elem* elem = list -> head;

    for (size_t i = 0; i < pos - 1; i++)
    {
        CHECK_PTR (elem, NULL);

        elem = elem -> next;
    }

    return elem;
}

list_elem* list_find (list_t* list, data_t val)
{
    CHECK_PTR (list, NULL);

    list_elem* elem = list -> head;

    for (size_t i = 0; i < list -> size; i++)
    {
        CHECK_PTR (elem, NULL);

        if ((elem -> value) != val)
            elem = elem -> next;

        else
            break;
    }

    return elem;
}

/*
    returns -1 if error
    returns 0  if success
*/
int list_process (list_t* list,
                  int (*func)(data_t val, void* buf), void* buf)
{
    CHECK_PTR (list, -1);
    CHECK_PTR (func, -1);

    list_elem* elem = list -> head;
    size_t size = list -> size;

    for (size_t i = 0; i < size; i++)
    {
        CHECK_PTR (elem, -1);

        if ( ((*func)(elem -> value, buf)) == -1)
        {
            return -1;
        }

        elem = elem -> next;
    }

    return 0;
}
