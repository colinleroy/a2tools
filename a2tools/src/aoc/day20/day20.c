#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "extended_conio.h"

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

void debug_array(item *cur, int n, int num_lines) {
    item *start = cur;
    int i = 0;
    return;
    while (i ++ < num_lines) {
      printf("%5d\n", cur->value);
      cur = cur->next;
    }
    printf("\n");
    cgetc();
    // if (n < 10 || 
    //    (n < 100 && n % 10 == 0) ||
    //    (n % 100 == 0) ||
    //    n == num_lines) {
    //   for (i = 0; i < 3; i++)
    //     start = start->prev;
    // 
    //   gotoxy(0, 10);
    //   for (i = 0; i < 6; i++) {
    //     printf("%5d ", start->value);
    //     start = start->next;
    //   }
    //   printf("\n\n%5d shifts/%5d done\n", n, num_lines);
    // }
    
}

static item *remove_after;
static item *remove_before;
static item *insert_before;
static item *insert_after;

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  item *values = NULL;
  int i = 0, shifted = 0, num_lines = 0, sum = 0;
  item *cur, *start, *zero, *last_read = NULL;
  item *next_to_shift;

  clrscr();
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    values = malloc(sizeof(struct _item));
    values->value = atoi(buf);
    num_lines++;
    if (i == 0) {
      start = values;
    } else {
      values->prev = last_read;
      last_read->next = values;
      last_read->original_next = values;
    }
    last_read = values;
    if (i % 100 == 0) {
      gotoxy(0,0);
      printf("Reading... %d lines...\n", i);
    }
    i++;
  }
  free(buf);
  fclose(fp);

  last_read->original_next = NULL;
  last_read->next = start;
  start->prev = last_read;

  cur = start;
  debug_array(cur, 0, num_lines);
  do {
    int shift, rem;

    if (cur->value == 0) {
      zero = cur;
    }
    next_to_shift = cur->original_next;

    shift = cur->value;
    rem = shift;

    if (rem == 0)
      goto nothing_to_do;
    /* handle us leaving start */
    if (cur == start) {
      start = rem > 0 ? start->next : start->prev;
    }

    remove_after = cur->prev;
    remove_before = cur->next;

    insert_before = rem > 0 ? cur->next : cur->prev;
    /* Pluck it for where we are */
    cur->prev = NULL;
    cur->next = NULL;

    /* Join RA-RB */
    remove_after->next = remove_before;
    remove_before->prev = remove_after;

    if (rem > 0) {
      for (i = 0; i < rem; i++) {
        insert_before = insert_before->next;
      }
    }
    if (rem < 0) {
      for (i = rem; i < -1; i++) {
        insert_before = insert_before->prev;
      }
    }

    insert_after = insert_before->prev;
    
    /* we are now like:
     *  - IA - x - IB - ... - RA - CUR - RB - ... */
    
    /* handle us taking start's spot */
    if (insert_before->prev == start) {
      start = cur;
    }
    
    /* reinsert where we should */
    insert_after->next = cur;
    cur->next = insert_before;
    insert_before->prev = cur;
    cur->prev = insert_after;

nothing_to_do:
    shifted++;
    debug_array(start, shifted, num_lines);

    cur = next_to_shift;
  } while (next_to_shift != NULL);
  
  printf("\n");
  
  cur = zero;

  for (i = 0; i < 3001; i++) {
    if (i > 0 && i % 1000 == 0) {
      debug_array(start, num_lines, num_lines);

      sum += cur->value;
      gotoxy(0, 15);
      printf("Adding %d at %d\n", cur->value, i);
    }
    cur = cur->next;
  }

  printf("Code is: %d\n", sum);

}
