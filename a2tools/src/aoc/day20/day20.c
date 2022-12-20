#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "bool_array.h"

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN20"

int main(void) {
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  read_file(fp);

  exit (0);
}

typedef struct _item item;
struct _item {
  int value;
  item *prev;
  item *next;
  item *original_next;
};

void debug_array(item *cur) {
    item *start = cur;
    int i;
    for (i = 0; i < 3; i++)
      start = start->prev;
    for (i = 0; i < 6; i++) {
      printf("%5d ", start->value);
      start = start->next;
    }
    printf("\n");
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  item *values = NULL;
  int i = 0, shifted = 0, j = 0, sum = 0;
  item *cur, *start, *zero, *last_read = NULL;
  item *next_to_shift;

  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    values = malloc(sizeof(struct _item));
    values->value = atoi(buf);
    if (i == 0) {
      start = values;
    } else {
      values->prev = last_read;
      last_read->next = values;
      last_read->original_next = values;
    }
    last_read = values;
    i++;
  }
  free(buf);
  fclose(fp);

  last_read->original_next = NULL;
  last_read->next = start;
  start->prev = last_read;

  cur = start;
  debug_array(cur);
  do {
    int shift, rem;
    if (cur->value == 0) {
      zero = cur;
    }
    next_to_shift = cur->original_next;

    shift = cur->value;
    rem = shift;
    while (rem > 0) {
      item *prev;
      item *next;
      item *after_next;

      prev = cur->prev;
      next = cur->next;
      after_next = next->next;

      if (cur == start) {
        start = next;
      }

      cur->prev = NULL;
      cur->next = NULL;
      
      prev->next = next;
      next->prev = prev;
      
      next->next = cur;
      cur->prev = next;
      
      cur->next = after_next;
      after_next->prev = cur;
      
      //debug_array(cur);
      rem--;
    }
    while (rem < 0) {
      item *prev = cur->prev;
      item *next = cur->next;
      item *before_prev = prev->prev;

      if (cur == start) {
        start = prev;
      }

      cur->prev = NULL;
      cur->next = NULL;

      next->prev = prev;
      prev->next = next;

      prev->prev = cur;
      cur->next = prev;
      
      before_prev->next = cur;
      cur->prev = before_prev;
      
      //debug_array(cur);
      rem++;
    }

    cur = next_to_shift;
    shifted++;
  } while (next_to_shift != NULL);
  printf("\n");
  
  cur = zero;

  for (i = 0; i < 3001; i++) {
    if (i > 0 && i % 1000 == 0) {
      debug_array(cur);

      sum += cur->value;
      printf("Adding %d at %d\n", cur->value, i);
    }
    cur = cur->next;
  }

  printf("value: %d\n", sum);

}
