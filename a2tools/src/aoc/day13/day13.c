#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "slist.h"


#define DATASET "IN13"
#define BUFSIZE 255
#define DEBUG

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

static void element_dump(element *e, int depth) {
  slist *w;
  if (e->value != -1) {
    printf("%d", e->value);
  } else {
    printf("[");
    for (w = e->list; w; w = w->next) {
      element_dump(w->data, depth + 1);
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

      values = slist_append(values, parse(parser_cur, &parser_cur));
    }
    if (out_cur != NULL) {
      *out_cur = parser_cur;
    }
    return element_new(-1, values);
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
  ORDERED,
  UNSURE,
  NOT_ORDERED,
  UNDETERMINED
};

static int compare(element *l, element *r, int depth) {
  int result = UNDETERMINED;
  int i;
  
  printf("\n");
  for (i = 0; i < depth; i++) {
    printf(" ");
  }
  printf("Comparing ");
  element_dump(l, 0); 
  printf(" / ");
  element_dump(r, 0);

  if (l->value == -1) {
    slist *l_w = l->list;
    if (r->value == -1) {
      slist *r_w = r->list;
      
      while (l_w && r_w) {
        result = compare(l_w->data, r_w->data, depth + 1);
        if (result != UNSURE) {
          goto out;
        }
        l_w = l_w->next;
        r_w = r_w->next;
      }
      /* still unsure ? which list is done ?*/
      if (l_w == NULL && r_w != NULL) {
        printf(" left out of elements ");
        result = ORDERED;
        goto out;
      } else if (l_w != NULL && r_w == NULL) {
        printf(" right out of elements ");
        result = NOT_ORDERED;
        goto out;
      } else {
        printf(" both out of elements ");
        result = UNSURE;
        goto out;
      }
      
    } else {
      /* make a list out of r */
      slist *new_r_list = slist_append(NULL, element_new(r->value, NULL));
      element *new_r = element_new(-1, new_r_list);

      result = compare(l, new_r, depth + 1);
      element_free(new_r);

      goto out;
    }
  } else {
    if (r->value == -1) {
      /* make a list out of l */
      slist *new_l_list = slist_append(NULL, element_new(l->value, NULL));
      element *new_l = element_new(-1, new_l_list);

      result = compare(new_l, r, depth + 1);
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
  for (i = 0; i < depth; i++) {
    printf(" ");
  }
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
    case UNDETERMINED:
      printf(" => UNDETERMINED\n");
      break;
  }
  return result;
}

static void read_file(FILE *fp) {
  element *l_val, *r_val;
  int cur_idx = 1;
  long sum = 0;
  char l_buf[BUFSIZE];
  char r_buf[BUFSIZE];
  char skip[2];

  do {
    fgets(l_buf, sizeof(l_buf), fp);
    l_val = parse(l_buf, NULL);
    fgets(r_buf, sizeof(r_buf), fp);
    r_val = parse(r_buf, NULL);
    fgets(skip, sizeof(skip), fp);
    
    printf("\nNew line: ");

    if (compare(l_val, r_val, 0) != NOT_ORDERED) {
      sum += cur_idx;
    }
    
    element_free(l_val);
    element_free(r_val);
    
    cur_idx++;
  } while (!feof(fp));
  printf("Sum is: %ld\n", sum);
}
