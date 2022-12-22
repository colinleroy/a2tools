#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
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
static int n_monkeys = 0;
static short humn_idx;
static bigint *get_result_for(short m_idx);

static monkey *read_monkey(short idx) {
  monkey *m = malloc(sizeof(struct _monkey));
  memset(m, 0, sizeof(struct _monkey));
  fseek(mfp, idx * sizeof(struct _monkey), SEEK_SET);
  printf("reading monkey %d at %ld\n", idx, idx * sizeof(struct _monkey));
  if (fread(m, sizeof(struct _monkey), 1, mfp) < 1) {
    printf("error reading: %s\n",strerror(errno));
  }
  
  return m;
}

static bigint *compute(short left_idx, char operator, short right_idx) {
  bigint *left_r, *right_r;
  bigint *result = NULL;

  left_r = get_result_for(left_idx);
  right_r = get_result_for(right_idx);

  printf("%s %c %s = ", left_r, operator, right_r);
  switch(operator) {
    case '+':
      result = bigint_add(left_r, right_r);
      break;
      ;;
    case '-':
      result = bigint_sub(left_r, right_r);
      break;
      ;;
    case '*':
      result = bigint_mul(left_r, right_r);
      break;
      ;;
    case '/':
      result = bigint_div(left_r, right_r);
      break;
      ;;
    default:
      printf("Unknown operator %c\n", operator);
      exit(1);
  }
  printf("%s\n", result);
  free(left_r);
  free(right_r);
  return result;
}
static int seen_humn = 0;
static bigint *get_result_for(short m_idx) {
  bigint *result;
  monkey *m = read_monkey(m_idx);
  if (m_idx == humn_idx)
    seen_humn = 1;
  if (m->number_or_operator != 0xFF) {
    result = compute(m->left_operand, m->number_or_operator, m->right_operand);
  } else {
    result = bigint_new_from_long(m->number);
  }
  free(m);
//  printf("Result for %d: %s\n", m_idx, result);

  return result;
}

#define LEFT 0
#define RIGHT 1

static char rbuf[255];
static void read_file(FILE *fp) {
  bigint *p1res;
  short root_idx;
  
  /* do we have cached data ? (we'd only need indexes )*/
  mfp = fopen("MKYS", "rb");
  if (mfp == NULL) {
    printf("Data absent\n");
    exit(1);
  }
  while (fgets(rbuf, BUFSIZE-1, fp) != NULL) {
    *strchr(rbuf, ':') = '\0';
    if (!strcmp(rbuf, "root")) {
      root_idx = n_monkeys;
    } else if (!strcmp(rbuf, "humn")) {
      humn_idx = n_monkeys;
    } 
    printf("Read monkey %d named %s\n", n_monkeys, rbuf);
    n_monkeys++;
  }

  fclose(fp);

  p1res = get_result_for(root_idx);
  printf("part1: %s\n", p1res);
  free(p1res);

  fclose(mfp);
}
