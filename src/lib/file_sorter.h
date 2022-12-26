#include <inttypes.h>
#include "array_sort.h"

#ifndef __file_sorter_h
#define __file_sorter_h

typedef struct _file_sorter file_sorter;

typedef void * (*read_object_func) (FILE *fp);
typedef void   (*write_object_func) (void *, FILE *fp);
typedef void   (*free_func) (void *);

/* setups a file sorter
 * object compare: a standard sort_func (see array_sort.c)
 * object_read: function to read object from file. Optional,
 *              defaults to reading a \n-delimited string
 *              from file
 * object_write: function to read object from file. Optional,
 *               defaults to writing a \n-delimited string
 *               to file
 * object_write: function to read object from file. Optional,
 *               defaults to writing a \n-delimited string
 *               to file
 * object_free:  function to free object. Optional, defaults
 *               to free()
 */
file_sorter * file_sorter_new(sort_func object_compare,
                    read_object_func object_read,
                    write_object_func object_write,
                    free_func object_free);

/* adds data to sort */
int file_sorter_add_data(file_sorter *sorter, int n_objects, void **objects);

/* sort data, return the sorted file */
char *file_sorter_sort_data(file_sorter *sorter);

void file_sorter_free(file_sorter *sorter);

#endif
