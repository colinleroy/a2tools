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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "slist.h"
#include "array_sort.h"

#define DATASET "IN13"
#define BUFSIZE 255

static void read_file(FILE *fp);

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

  fclose(fp);

  exit (0);
}

typedef struct _element element;
struct _element {
  int value;
  slist *list;
};

static element *element_new(int value, slist *list) {
  element *e = malloc(sizeof(element));
  e->value = value;
  e->list = list;
  return e;
}

static void element_dump(element *e) {
  slist *w;
  if (e->value != -1) {
    printf("%d", e->value);
  } else {
    printf("[");
    for (w = e->list; w; w = w->next) {
      element_dump(w->data);
      if (w->next)
        printf(",");
    }
    printf("]");
  }
}

static void element_free(element *e) {
  slist *w;
  for (w = e->list; w; w = w->next)
    element_free(w->data);
  slist_free(e->list);
  free(e);
}

static element *parse(char *parser_cur, char **out_cur) {
  int is_array;
  char c = *(parser_cur++);
  
  is_array = (c == '[');

  if (is_array) {
    slist *values = NULL;
    while((c = *(parser_cur++)) != ']') {
      if (c != ',')
        parser_cur--;

      values = slist_prepend(values, parse(parser_cur, &parser_cur));
    }
    if (out_cur != NULL) {
      *out_cur = parser_cur;
    }
    return element_new(-1, slist_reverse(values));
  } else {
    int val = c - '0';
    while ((c = *(parser_cur++)) >= '0' && c <= '9') {
      val *= 10;
      val += c - '0';
    }
    parser_cur--;
    if (out_cur != NULL) {
      *out_cur = parser_cur;
    }
    return element_new(val, NULL);
  }
}

enum {
  ORDERED       = -1,
  UNSURE        = 0,
  NOT_ORDERED   = 1
};

static int compare(void *a, void *b) {
  element *l = (element *)a;
  element *r = (element *)b;
  int result;

#ifdef DEBUG  
  printf("\nComparing ");
  element_dump(l); 
  printf(" / ");
  element_dump(r);
#endif

  if (l->value == -1) {
    slist *l_w = l->list;
    if (r->value == -1) {
      slist *r_w = r->list;
      
      while (l_w && r_w) {
        result = compare(l_w->data, r_w->data);
        if (result != UNSURE) {
          goto out;
        }
        l_w = l_w->next;
        r_w = r_w->next;
      }
      /* still unsure ? which list is done ?*/
      if (l_w == NULL && r_w != NULL) {
#ifdef DEBUG  
        printf(" left out of elements ");
#endif
        result = ORDERED;
        goto out;
      } else if (l_w != NULL && r_w == NULL) {
#ifdef DEBUG  
        printf(" right out of elements ");
#endif
        result = NOT_ORDERED;
        goto out;
      } else {
#ifdef DEBUG  
        printf(" both out of elements ");
#endif
        result = UNSURE;
        goto out;
      }
      
    } else {
      /* make a list out of r */
      slist *new_r_list = slist_append(NULL, element_new(r->value, NULL));
      element *new_r = element_new(-1, new_r_list);

      result = compare(l, new_r);
      element_free(new_r);

      goto out;
    }
  } else {
    if (r->value == -1) {
      /* make a list out of l */
      slist *new_l_list = slist_append(NULL, element_new(l->value, NULL));
      element *new_l = element_new(-1, new_l_list);

      result = compare(new_l, r);
      element_free(new_l);

      goto out;
    } else {
      /* integer compare */
      if (l->value < r->value) {
        result = ORDERED;
        goto out;
      } else if (l->value > r->value) {
        result = NOT_ORDERED;
        goto out;
      } else {
        result = UNSURE;
        goto out;
      }
    }
  }
  printf("How are we there!?\n");
  exit(1);
out:
#ifdef DEBUG
  switch (result) {
    case ORDERED:
      printf(" => ORDERED\n");
      break;
    case NOT_ORDERED:
      printf(" => NOT_ORDERED\n");
      break;
    case UNSURE:
      printf(" => UNSURE\n");
      break;
  }
#endif
  return result;
}

char l_buf[BUFSIZE];
char r_buf[BUFSIZE];
static void read_file(FILE *fp) {
  element *l_val, *r_val;
  int cur_idx = 1;
  long sum = 0;
  char skip[2];
  element **packets = NULL, **sorted = NULL;
  element *first_div, *second_div;
  int first_div_idx, second_div_idx;

  first_div = parse("[[2]]", NULL);
  second_div = parse("[[6]]", NULL);
  first_div_idx = 1;
  /* one more because it's going to be after "[[2]]" */
  second_div_idx = first_div_idx + 1;

  do {
    fgets(l_buf, sizeof(l_buf), fp);
    fgets(r_buf, sizeof(r_buf), fp);

    l_val = parse(l_buf, NULL);
    r_val = parse(r_buf, NULL);

    fgets(skip, sizeof(skip), fp);

    if (compare(l_val, r_val) == ORDERED) {
      sum += cur_idx;
    }
    if (compare(l_val, first_div) == ORDERED) {
      first_div_idx++;
    }
    if (compare(r_val, first_div) == ORDERED) {
      first_div_idx++;
    }
    if (compare(l_val, second_div) == ORDERED) {
      second_div_idx++;
    }
    if (compare(r_val, second_div) == ORDERED) {
      second_div_idx++;
    }

    cur_idx++;
  } while (!feof(fp));

  element_free(first_div);
  element_free(second_div);

  printf("Sum is: %ld\n", sum);


  printf("Decoder key: %d * %d = %d\n", first_div_idx, second_div_idx, first_div_idx * second_div_idx);
}
