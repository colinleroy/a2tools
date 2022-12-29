#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2enh.h>
#include <conio.h>
#endif
#include "bigint.h"
#include "extended_conio.h"

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN21"

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

typedef struct _monkey monkey;
struct _monkey {
  short number;
  short number_or_operator;
  short left_operand;
  short right_operand;
};

typedef struct _monkey_name monkey_name;
struct _monkey_name {
  char name[5];
};

#define SIZEOF_PTR 2

//static monkey *monkeys = NULL;
static FILE *mfp = NULL;
static monkey_name *monkey_names = NULL;
static int n_monkeys = 0;

/* Fixme optimize that */
static int get_monkey_index(const char *name) {
  int i;
  for (i = 0; i < n_monkeys; i++) {
    if (!strcmp(monkey_names[i].name, name)) {
      return i;
    }
  }
  return -1;
}

static char rbuf[255];
static void read_file(FILE *fp) {
  short monkey_index = 0;
  
  while (fgets(rbuf, BUFSIZE-1, fp) != NULL) {
    *strchr(rbuf, ':') = '\0';
    monkey_names = realloc(monkey_names, (n_monkeys + 1) * sizeof(struct _monkey_name));
    strcpy(monkey_names[n_monkeys].name, rbuf);
    printf("Read monkey %d named %s\n", n_monkeys, monkey_names[n_monkeys].name);
    n_monkeys++;
  }

  rewind(fp);
  mfp = fopen("MKYS", "w+b");

  while (fgets(rbuf, BUFSIZE-1, fp) != NULL) {
    
    char *name, *number_str, *left_operand, *operator, *right_operand;
    int has_number;
    monkey *m = malloc(sizeof(struct _monkey));
    name = rbuf;
    number_str = strchr(rbuf, ' ') + 1;

    *strchr(name, ':') = '\0';
    // printf("Building monkey %s at %d: ", name, monkey_index);
    if (strchr(number_str, ' ')) {
      left_operand = number_str;
      operator = strchr(left_operand, ' ') + 1;
      right_operand = strchr(operator, ' ') +1;
      
      *strchr(left_operand, ' ') = '\0';
      *strchr(operator, ' ') = '\0';
      *strchr(right_operand, '\n') = '\0';

      has_number = 0;
      // printf("%s %s %s", left_operand, operator, right_operand);
    } else {
      *strchr(number_str, '\n') = '\0';
      left_operand = NULL;
      operator = NULL;
      right_operand = NULL;
      has_number = 1;
      // printf("%s", number_str);
    }

    if (has_number == 0) {
      m->number = 0;
      m->number_or_operator = *operator;
      m->left_operand = get_monkey_index(left_operand);
      m->right_operand = get_monkey_index(right_operand);
    } else {
      m->number = atoi(number_str);
      m->number_or_operator = 0xFF;
      m->left_operand = -1;
      m->right_operand = -1;
    }
    printf("writing monkey %d at %ld\n", monkey_index, ftell(mfp));
    fwrite(m, sizeof(struct _monkey), 1, mfp);
    free(m);
    // printf(". done\n");
    monkey_index++;
  }

  fclose(mfp);
  fclose(fp);

  printf("Data written.\n");
}
