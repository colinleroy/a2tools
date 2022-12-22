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
static monkey_name *monkey_names = NULL;
static int n_monkeys = 0;
static short root_idx, humn_idx;
static bigint *get_result_for(short m_idx);

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

static bigint *invert_op(short start_idx, bigint *outcome) {
  bigint *result_to_change, *result_to_keep, *res, *tmp;
  int side, investigate_idx;
  monkey *m = read_monkey(start_idx);
  seen_humn = 0;
  res = get_result_for(m->left_operand);
  if (seen_humn == 0) {
    result_to_keep = res;
    result_to_change = get_result_for(m->right_operand);
    investigate_idx = m->right_operand;
    side = RIGHT;
  } else {
    result_to_keep = get_result_for(m->right_operand);
    result_to_change = res;
    investigate_idx = m->left_operand;
    side = LEFT;
  }
  if (side == LEFT) {
    //printf("we get at %s by having >>%s<< %c %s : ", outcome, result_to_change, monkeys[start_idx].number_or_operator, result_to_keep);
    switch(m->number_or_operator) {
      case '+': res = bigint_sub(outcome, result_to_keep); break;
      case '-': res = bigint_add(outcome, result_to_keep); break;
      case '*': res = bigint_div(outcome, result_to_keep); break;
      case '/': res = bigint_mul(outcome, result_to_keep); break;
    }
    //printf("%s\n", res);
  } else {
    //printf("we get at %s by changing %s %c >>%s<< : ", outcome, result_to_keep, monkeys[start_idx].number_or_operator, result_to_change);
    switch(m->number_or_operator) {
      case '+': res = bigint_sub(outcome, result_to_keep); break;
      case '-': res = bigint_sub(result_to_keep, outcome); break;
      case '*': res = bigint_div(outcome, result_to_keep); break;
      case '/': res = bigint_mul(outcome, result_to_keep); break;
    }
    //printf("%s\n", res);
  }
  free(result_to_keep);
  free(result_to_change);
  free(m);
  if (investigate_idx != humn_idx) {
    tmp = invert_op(investigate_idx, res);
    free(res);
    return tmp;
  } else
    return res;
}

static char rbuf[255];
static void read_file(FILE *fp) {
  bigint *p1res, *p2res;
  short root_idx;
  short monkey_index = 0;
  short investigate_idx = -1;
  bigint *ref_result;
  monkey *root;
  
  /* do we have cached data ? (we'd only need indexes )*/
  mfp = fopen("MKYS", "rb");
  
  while (fgets(rbuf, BUFSIZE-1, fp) != NULL) {
    *strchr(rbuf, ':') = '\0';
    monkey_names = realloc(monkey_names, (n_monkeys + 1) * sizeof(struct _monkey_name));
    strcpy(monkey_names[n_monkeys].name, rbuf);
    if (!strcmp(monkey_names[n_monkeys].name, "root")) {
      root_idx = n_monkeys;
    } else if (!strcmp(monkey_names[n_monkeys].name, "humn")) {
      humn_idx = n_monkeys;
    } 
    printf("Read monkey %d named %s\n", n_monkeys, monkey_names[n_monkeys].name);
    n_monkeys++;
  }

  if (mfp != NULL)
    goto skip_data_parse;

  rewind(fp);
  mfp = fopen("MKYS", "w+b");

  while (fgets(rbuf, BUFSIZE-1, fp) != NULL) {
    
    char *name, *number_str, *left_operand, *operator, *right_operand;
    int has_number;
    char *ref_result;
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

  rewind(mfp);

skip_data_parse:

  /* No need for names anymore */
  free(monkey_names);
  fclose(fp);

  p1res = get_result_for(root_idx);
  printf("part1: %s\n", p1res);
  free(p1res);

  // cgetc();
  // printf("\n");
  // seen_humn = 0;
  // root = read_monkey(root_idx);
  // p2res = get_result_for(root->left_operand);
  // if (seen_humn == 1) {
  //   free(p2res);
  //   ref_result = get_result_for(root->right_operand);
  //   investigate_idx = root->left_operand;
  // } else {
  //   ref_result = p2res;
  //   investigate_idx = root->right_operand;
  // }
  // free(root);
  // 
  // p2res = invert_op(investigate_idx, ref_result);
  // free(ref_result);
  // 
  // printf("part2: %s\n", p2res);
  // free(p2res);

  fclose(mfp);
}
