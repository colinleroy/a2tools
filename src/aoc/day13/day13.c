#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "slist.h"
#include "array_sort.h"
#include "file_sorter.h"

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
  if (e == NULL) {
    printf("couldn't allocate element\n");
    return NULL;
  }
  e->value = value;
  e->list = list;
  return e;
}

static void element_dump(element *e, FILE *fp) {
  slist *w;
  if (fp == NULL) {
    fp = stdout;
  }

  if (e->value != -1) {
    fprintf(fp, "%d", e->value);
  } else {
    fprintf(fp, "[");
    for (w = e->list; w; w = w->next) {
      element_dump(w->data, fp);
      if (w->next)
        fprintf(fp, ",");
    }
    fprintf(fp, "]");
  }
}

static void element_dump_ln(element *e, FILE *fp) {
  element_dump(e, fp);
  fprintf(fp, "\n");
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
  element_dump(l, NULL); 
  printf(" / ");
  element_dump(r, NULL);
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

char buf[BUFSIZE];
static element *read_element(FILE *fp){
  buf[0] = '\0';
  fgets(buf, sizeof(buf), fp);
  if (buf[0] != '\0') {
    return parse(buf, NULL);
  }
  return NULL;
}

static void read_file(FILE *fp) {
  element *l_val, *r_val;
  int cur_idx = 1;
  long sum = 0;
  char skip[2];
  int i = 0;
  int num_files = 0, lines_in_file = 0;
  element **packets = NULL, **sorted = NULL;
  char *sorted_file;
  file_sorter *sorter;
  element **divs = malloc(2*sizeof(element *));
  sum = 0;

   sorter = file_sorter_new(compare,
                  (read_object_func)read_element,
                  (write_object_func)element_dump_ln,
                  (free_func)element_free);
  do {
    if (lines_in_file > 18) {
flush_last_packets:
      file_sorter_add_data(sorter, lines_in_file, (void **)packets);
      for (i = 0; i < lines_in_file; i++) {
        element_free(packets[i]);
      }
      free(packets);
      packets = NULL;

      lines_in_file = 0;

      if (feof(fp)) {
        /* we're flushing */
        goto end_write_files;
      }
    }
    lines_in_file += 2;

    l_val = read_element(fp);
    r_val = read_element(fp);

    packets = realloc(packets, lines_in_file*sizeof(element *));
    if (packets == NULL) {
      printf("Couldn't allocate packet list\n");
    }
    packets[lines_in_file-2] = l_val;
    packets[lines_in_file-1] = r_val;

    fgets(skip, sizeof(skip), fp);

    if (compare(l_val, r_val) != NOT_ORDERED) {
      sum += cur_idx;
    }

    cur_idx++;
  } while (!feof(fp));
  
  goto flush_last_packets;

end_write_files:
  divs[0] = parse("[[2]]", NULL);
  divs[1] = parse("[[6]]", NULL);
  file_sorter_add_data(sorter, 2, (void **)divs);
  element_free(divs[0]);
  element_free(divs[1]);
  free(divs);
  printf("xSum is: %ld\n", sum);
  
  sorted_file = file_sorter_sort_data(sorter);
  fp = fopen(sorted_file, "r");
  cur_idx = 1;
  while (1) {
    fgets(buf, sizeof(buf), fp);
    if (!strcmp(buf,"[[2]]\n"))
      sum = cur_idx;
    if (!strcmp(buf,"[[6]]\n")) {
      sum *= cur_idx;
      break;
    }
    cur_idx++;
  }
  fclose(fp);
  unlink(sorted_file);
  file_sorter_free(sorter);
  free(sorted_file);
  printf("Code is %ld\n", sum);
}
