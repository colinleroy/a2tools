/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "array_sort.h"
#include "file_sorter.h"

#define BUFSIZE 255
enum {
  FILE_SORTER_ADDING_DATA,
  FILE_SORTER_SORTING_DATA
};

struct _file_sorter {
  int step;
  
  /* comparator */
  sort_func object_compare;
  read_object_func object_read;
  write_object_func object_write;
  free_func object_free;

  int num_input_files;
};

static char *fs_fgets_alloc(FILE *fp){
  char *buf = malloc(255);
  memset(buf, 0, 255);
  fgets(buf, 254, fp);
  
  return buf;
}
static void fs_fputs(char *data, FILE *fp) {
  fprintf(fp, "%s\n", data);
}

/* setups a file sorter */
file_sorter *file_sorter_new(sort_func object_compare,
                    read_object_func object_read,
                    write_object_func object_write,
                    free_func object_free) {

  file_sorter *sorter = malloc(sizeof(struct _file_sorter));

  if (sorter == NULL) {
    return NULL;
  }
  sorter->step = FILE_SORTER_ADDING_DATA;

  sorter->object_compare = object_compare;

  if (object_read != NULL) {
    sorter->object_read = object_read;
  } else {
    sorter->object_read = (read_object_func)fs_fgets_alloc;
  }

  if (object_write != NULL) {
    sorter->object_write = object_write;
  } else {
    sorter->object_write = (write_object_func)fs_fputs;
  }

  if (object_free != NULL) {
    sorter->object_free = object_free;
  } else {
    sorter->object_free = free;
  }

  sorter->num_input_files = 0;

  return sorter;
}

static char *get_filename(int num_file) {
  char *filename = malloc(BUFSIZE);

#ifdef __CC65__
  sprintf(filename, "SRT%d", num_file);
#else
  sprintf(filename, "/tmp/sorter-%d", num_file);
#endif
  return filename;
}

static FILE *open_filenum(int num_file, char *mode) {
  char *filename = get_filename(num_file);
  FILE *fp;

#ifdef __APPLE2ENH__
  _filetype = PRODOS_T_TXT;
#else
#endif
  printf("File sorter: Opening file %s for %s\n", filename, mode);
  fp = fopen(filename, mode);

  if (fp == NULL) {
    printf("Error opening %s %s: %d\n", filename, mode, errno);
  }
  free(filename);
  return fp;
}

static void delete_out(int num_file) {
  char *filename = get_filename(num_file);

  printf("File sorter: Deleting file %d\n", num_file);
  unlink(filename);
  free(filename);
}


/* adds n_objects object to a new file store.
 */
int file_sorter_add_data(file_sorter *sorter, int n_objects, void **objects) {
  int i;
  FILE *fp;
  void **sorted = objects;

  if (sorter->step != FILE_SORTER_ADDING_DATA) {
    return -1;
  }

  bubble_sort_array((void **)sorted, n_objects, sorter->object_compare);

  fp = open_filenum(sorter->num_input_files, "w");
  if (fp == NULL) {
    return -1;
  }
  sorter->num_input_files++;

  for (i = 0; i < n_objects; i++) {
    sorter->object_write(sorted[i], fp);
  }
  fclose(fp);

  return 0;
}

/* sort files from start to end - 1 */
static int sort_files(file_sorter *sorter, int start, int num_files) {
  int new_start = num_files;
  int created_files = new_start;
  int i;
  void *l_val = NULL, *r_val = NULL;
  int all_files_done = 0;

  if (start == num_files) {
    printf("File sorter: finished\n");
    return start;
  }

  for (i = start; i <= num_files; i+=2) {
    FILE *in1 = NULL, *in2 = NULL, *out;

    in1 = open_filenum(i, "r");
    if (i + 1 <= num_files)
      in2 = open_filenum(i + 1, "r");

    if (in2 == NULL) {
      all_files_done = 1;
    }
    created_files++;
    out = open_filenum(created_files, "w");

    do {
      if (in1 != NULL && l_val == NULL && !feof(in1)) {
        l_val = sorter->object_read(in1);
      }
      if (in2 != NULL && r_val == NULL && !feof(in2)) {
        r_val = sorter->object_read(in2);
      }

      if (l_val != NULL && r_val != NULL) {
        if (sorter->object_compare(l_val, r_val) <= 0) {
            sorter->object_write(l_val, out);
            sorter->object_free(l_val);
            l_val = NULL;
        } else {
            sorter->object_write(r_val, out);
            sorter->object_free(r_val);
            r_val = NULL;
        }
      } else if (l_val != NULL) {
            sorter->object_write(l_val, out);
            sorter->object_free(l_val);
        l_val = NULL;
      } else if (r_val != NULL) {
            sorter->object_write(r_val, out);
            sorter->object_free(r_val);
        r_val = NULL;
      } else {
        break;
      }
    } while (1);

    if (in1 != NULL) {
      fclose(in1); 
    }
    in1 = NULL;
    if (in2 != NULL) {
      fclose(in2);
    }
    in2 = NULL;
    fclose(out);
    delete_out(i);
    if (i + 1 <= num_files){
      delete_out(i+1);
    }
  }
  if (start < num_files) {
    return sort_files(sorter, new_start + 1, created_files);
  }
}


/* sort data, return the sorted file */
char *file_sorter_sort_data(file_sorter *sorter) {
  int final_file = sort_files(sorter, 0, sorter->num_input_files);
  return get_filename(final_file);
}

void file_sorter_free(file_sorter *sorter) {
  if (sorter == NULL) {
    return;
  }
  free(sorter);
}
