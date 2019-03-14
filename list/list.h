#ifndef __LIST_H__
#define __LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct list_t       list_t;
typedef struct list_elem    list_elem;
typedef int                 data_t;

#define RESERVE_VALUE 0

list_t*     list_constr();
int         list_push_tail  (list_t* list, data_t val);
int         list_push_head  (list_t* list, data_t val);
int         list_insert     (list_t* list, list_elem* pos, data_t val);
int         list_check_elem (list_t* list, list_elem* elem);
int         list_del        (list_t* list, list_elem* elem);
list_elem*  list_get_head   (list_t* list);
list_elem*  list_get_tail   (list_t* list);
list_elem*  list_get        (list_t* list, size_t pos);
list_elem*  list_find       (list_t* list, data_t val);

#endif //__LIST_H__
