#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
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

static FILE *open_out(int num_file, char *mode) {
  char *filename = malloc(BUFSIZE);
  FILE *fp;

  sprintf(filename, "out%d", num_file);

  printf("Opening file %d for %s\n", num_file, mode);

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(filename, mode);
  if (fp == NULL) {
    printf("Error opening %s %s: %d\n", filename, mode, errno);
    exit(1);
  }
  free(filename);
  return fp;
}

static void delete_out(int num_file) {
  char *filename = malloc(BUFSIZE);

  printf("Deleting file %d\n", num_file);
  sprintf(filename, "out%d", num_file);
  unlink(filename);
  free(filename);
}

char buf[BUFSIZE];
static int sort_files(int start, int num_files) {
  int new_start = num_files;
  int created_files = new_start;
  int i;
  element *l_val = NULL, *r_val = NULL;

  if (start == num_files) {
    printf("all done!");
    return start;
  }

  for (i = start; i <= num_files; i+=2) {
    FILE *in1, *in2, *out;

    in1 = open_out(i, "r");
    if (i + 1 <= num_files)
      in2 = open_out(i + 1, "r");

    created_files++;
    out = open_out(created_files, "w");

    do {
      if (l_val == NULL && !feof(in1)) {
        buf[0] = '\0';
        fgets(buf, sizeof(buf), in1);
        if (buf[0] != '\0') {
          l_val = parse(buf, NULL);
        }
      }
      if (in2 != NULL && r_val == NULL && !feof(in2)) {
        buf[0] = '\0';
        fgets(buf, sizeof(buf), in2);
        if (buf[0] != '\0') {
          r_val = parse(buf, NULL);
        }
      }

      if (l_val != NULL && r_val != NULL) {
        if (compare(l_val, r_val) != NOT_ORDERED) {
            element_dump(l_val, out);
            element_free(l_val);
            l_val = NULL;
        } else {
            element_dump(r_val, out);
            element_free(r_val);
            r_val = NULL;
        }
      } else if (l_val != NULL) {
        element_dump(l_val, out);
        element_free(l_val);
        l_val = NULL;
      } else if (r_val != NULL) {
        element_dump(r_val, out);
        element_free(r_val);
        r_val = NULL;
      } else {
        break;
      }
      fprintf(out, "\n");
    } while (1);
    fclose(in1); 
    in1 = NULL;
    if (in2 != NULL) {
      fclose(in2);
      in2 = NULL;
    }
    fclose(out);
    delete_out(i);
    if (i + 1 <= num_files){
      delete_out(i+1);
    }
  }
  if (start < num_files) {
    return sort_files(new_start + 1, created_files);
  }
}

static void read_file(FILE *fp) {
  element *l_val, *r_val;
  int cur_idx = 1;
  long sum = 0;
  char skip[2];
  int i = 0;
  int num_files = 0, lines_in_file = 0;
  FILE *outfp;
  element **packets = NULL, **sorted = NULL;
  int sorted_file;

  sum = 0;

  do {
    if (lines_in_file > 18) {
flush_last_packets:
      printf("Sorting %d packets...\n", lines_in_file);
      sorted = packets;
      bubble_sort_array((void **)sorted, lines_in_file, compare);

      printf("Writing to file %d\n", num_files);
      outfp = open_out(num_files, "w");
      for (i = 0; i < lines_in_file; i++) {
        element_dump(sorted[i], outfp);
        fprintf(outfp, "\n");
        element_free(sorted[i]);
      }
      free(packets);
      packets = NULL;


      fclose(outfp);
      num_files++;
      lines_in_file = 0;

      if (feof(fp)) {
        /* we're flushing */
        goto end_write_files;
      }
    }
    lines_in_file += 2;

    fgets(buf, sizeof(buf), fp);
    l_val = parse(buf, NULL);
    fgets(buf, sizeof(buf), fp);
    r_val = parse(buf, NULL);

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
  outfp = open_out(num_files, "w");
  fputs("[[2]]\n", outfp);
  fputs("[[6]]\n", outfp);
  fclose(outfp);
  
  printf("Sum is: %ld\n", sum);
  
  sorted_file = sort_files(0, num_files);
  fp = open_out(sorted_file, "r");
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
  delete_out(sorted_file);
  printf("Code is %ld\n", sum);
}
