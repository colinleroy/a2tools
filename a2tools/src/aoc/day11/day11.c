#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extended_string.h"
#include "array_sort.h"
#include <errno.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "day11.h"

/* prototypes */
static void send_item(long item, monkey *m);
static void free_monkeys(int num);
static void free_monkey(monkey *monkey);
static void dump_monkey(int num, monkey *monkey);

#define NUM_ROUNDS 10000
#define HAVE_RELIEF 0

int debug_turn = 0;
int debug_round = 0;

long lcm;

char line[255];
monkey *monkeys[8];
int my_worry_level = 0;

static int read_file(FILE *fp) {
  int n_tokens, i;
  int num_monkey = 0;
  int parse_state = PARSE_MONKEY;

  while (fgets(line, sizeof(line), fp) != NULL) {
    char *trimmed = trim(line);
    char **tokens = NULL;

    n_tokens = strsplit(trimmed, ' ', &tokens);
    free(trimmed);

    switch(parse_state) {
      case PARSE_MONKEY:
        monkeys[num_monkey] = malloc(sizeof(monkey));
        memset(monkeys[num_monkey], 0, sizeof(monkey));
        monkeys[num_monkey]->monkey_num = num_monkey;
        break;
      case PARSE_ITEMS:
        for (i = ITEMS_START_TOKEN; i < n_tokens; i++) {
          if (strchr(tokens[i], ','))
            *strchr(tokens[i], ',') = '\0';
          send_item(atoi(tokens[i]), monkeys[num_monkey]);
        }
        break;
      case PARSE_OPERATION:
        monkeys[num_monkey]->operation = tokens[OPERATION_TOKEN][0];

        if (tokens[OPERATION_LEFT_OPERAND_TOKEN][0] >= '0'
         && tokens[OPERATION_LEFT_OPERAND_TOKEN][0] <= '9')
          monkeys[num_monkey]->left_operand = atoi(tokens[OPERATION_LEFT_OPERAND_TOKEN]);
        else
          monkeys[num_monkey]->left_operand_old = 1;

        if (tokens[OPERATION_RIGHT_OPERAND_TOKEN][0] >= '0'
         && tokens[OPERATION_RIGHT_OPERAND_TOKEN][0] <= '9')
          monkeys[num_monkey]->right_operand = atoi(tokens[OPERATION_RIGHT_OPERAND_TOKEN]);
        else
          monkeys[num_monkey]->right_operand_old = 1;

        break;
      case PARSE_TEST_CONDITION:
        monkeys[num_monkey]->test_operation = strdup(tokens[TEST_OPERATION_TOKEN]);
        monkeys[num_monkey]->test_operand = atoi(tokens[TEST_OPERAND_TOKEN]);
        break;
      case PARSE_TEST_TRUE:
        monkeys[num_monkey]->test_true_monkey = atoi(tokens[DEST_MONKEY_TOKEN]);
        break;
      case PARSE_TEST_FALSE:
        monkeys[num_monkey]->test_false_monkey = atoi(tokens[DEST_MONKEY_TOKEN]);
        break;
      case PARSE_EMPTY:
        break;
    }

    parse_state ++;
    if (parse_state == PARSE_DONE) {
      parse_state = PARSE_MONKEY;
      dump_monkey(num_monkey, monkeys[num_monkey]);
      num_monkey++;
    }

    for (i = 0; i < n_tokens; i++) {
      free(tokens[i]);
    }
    
    free(tokens);
  }
  dump_monkey(num_monkey, monkeys[num_monkey]);
  return num_monkey + 1;
}

static void send_item(long item, monkey *m) {
  m->n_items++;
  m->items = realloc(m->items, m->n_items * sizeof(long));
  m->items[m->n_items - 1] = item;
}

static void do_monkey_turn(int num_monkey) {
  int i = 0;
  monkey *m = monkeys[num_monkey];
  monkey *dest_monkey;
  int num_dest_monkey;

  if (debug_turn) printf("Monkey %d:\n", num_monkey);
  
  /* inspect and throw items, in the order listed. */
  for (i = 0; i < m->n_items; i++) {
    long item_worry_level = m->items[i];
    long left_operand = m->left_operand_old == 1 ? item_worry_level : m->left_operand;
    long right_operand = m->right_operand_old == 1 ? item_worry_level : m->right_operand;

    if (debug_turn) printf("  Monkey inspects an item with a worry level of %ld.\n", item_worry_level);
    m->num_inspections++;
    switch(m->operation) {
      case '+':
        item_worry_level = (long)left_operand + (long)right_operand;
        if (debug_turn) printf("    Worry level increases by %ld to %ld.\n", right_operand, item_worry_level);
        break;
      case '*':
        item_worry_level = (long)left_operand * (long)right_operand;
        if (debug_turn) printf("    Worry level is multiplied by %ld to %ld.\n", right_operand, item_worry_level);
        break;
      case '-':
        item_worry_level = (long)left_operand - (long)right_operand;
        if (debug_turn) printf("    Worry level decreases by %ld to %ld.\n", right_operand, item_worry_level);
        break;
      case '/':
        item_worry_level = (long)left_operand / (long)right_operand;
        if (debug_turn) printf("    Worry level is divided by %ld to %ld.\n", right_operand, item_worry_level);
        break;
    }
    if (item_worry_level < 0) {
      printf("ERROR monkey %d, item %d: %ld %c %ld = %ld\n", num_monkey, i, left_operand, m->operation, right_operand, item_worry_level);
      exit(1);
    }

#if HAVE_RELIEF
    /* I get relieved */
    item_worry_level = (long)item_worry_level / (long)3;
    if (debug_turn) printf("    Monkey gets bored with item. Worry level is divided by 3 to %ld\n",
           item_worry_level);
#else
    item_worry_level = (long)item_worry_level % (long)lcm;
    if (debug_turn) printf("    Worry level is divided by %ld to %ld for sanity\n",
         lcm, item_worry_level);
#endif

    /* Monkey ponders who to send to */
    dest_monkey = NULL;
    switch(m->test_operation[0]) {
      case 'd':
        if ((int)((long)item_worry_level % (long)m->test_operand) == 0) {
          /* Monkey sends to 'true' monkey */
          if (debug_turn) printf("    Current worry level is divisible by %d.\n", m->test_operand);
          num_dest_monkey = m->test_true_monkey;
          dest_monkey = monkeys[num_dest_monkey];
        } else {
          /* Monkey sends to 'false' monkey */
          if (debug_turn) printf("    Current worry level is not divisible by %d.\n", m->test_operand);
          num_dest_monkey = m->test_false_monkey;
          dest_monkey = monkeys[num_dest_monkey];
        }
        break;
    }
    /* Send item */
    send_item(item_worry_level, dest_monkey);
    if (debug_turn) printf("    Item with worry level %ld is thrown to monkey %d.\n",
           item_worry_level, num_dest_monkey);
  }
  
  /* Monkey sent all items */
  free(m->items);
  m->items = NULL;
  m->n_items = 0;
}

static void do_round(int num_monkeys) {
  int i;
  for (i = 0; i < num_monkeys; i++) {
    do_monkey_turn(i);
  }
  if (debug_round) {
    for (i = 0; i < num_monkeys; i++) {
      int j;
      monkey *m = monkeys[i];

      printf("Monkey %d: ", i);
      for (j = 0; j < m->n_items; j++) {
        printf("%ld, ",m->items[j]);
      }
      printf("\n");
    }
    printf("\n");
  }
}

static int monkey_activity_sorter(void *monkey_a, void *monkey_b) {
  monkey *a = (monkey *)monkey_a;
  monkey *b = (monkey *)monkey_b;

  return a->num_inspections > b->num_inspections;
}

static void count_inspections(int num) {
  int i;
  
  bubble_sort_array((void **)&monkeys, num, monkey_activity_sorter);

  for (i = 0; i < num; i++)
    printf("Monkey %d inspected items %d times.\n", monkeys[i]->monkey_num,
           monkeys[i]->num_inspections);
  
  printf("Monkey business is: \n%ld * %ld = %ld\n",
          (long)monkeys[num-1]->num_inspections, (long)monkeys[num-2]->num_inspections,
          (long)monkeys[num-1]->num_inspections * (long)monkeys[num-2]->num_inspections);
}

static long find_lcm(int num_monkeys) {
  long max = 0;
  int i = 0;
  for (i = 0; i < num_monkeys; i++) {
    if (monkeys[i]->test_operand > max)
      max = monkeys[i]->test_operand;
  }
  printf("searching for LCM starting at %ld\n", max);
again:
  for (i = 0; i < num_monkeys; i++) {
    if ((long)max % (long)monkeys[i]->test_operand != 0) {
      if ((long)max % 10000U == 0) {
        printf("still searching... %ld\n", max);
      }
      max++;
      goto again;
    }
  }
  printf("found LCM: %ld\n", max);
  return max;
}

int main(void) {
  FILE *fp;
  int num_monkeys = 0;
  int num_round = 0;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen("IN11", "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }
  
  num_monkeys = read_file(fp);
  
  lcm = find_lcm(num_monkeys);

  /* Do a round */
  for (num_round = 0; num_round < NUM_ROUNDS; num_round++) {
    if (debug_round) {
      printf("Round %d\n", num_round + 1);
    }
    do_round(num_monkeys);
  }

  count_inspections(num_monkeys);
  free_monkeys(num_monkeys);

  fclose(fp);
  exit(0);
}

static void free_monkeys(int num) {
  int i;
  for (i = 0; i < num; i++)
    free_monkey(monkeys[i]);
}

static void free_monkey(monkey *monkey) {
  free(monkey->items);
  free(monkey->test_operation);
  free(monkey);
}

static void dump_monkey(int num, monkey *monkey) {
  int i;
  printf("=> Monkey %d\n", num);
  
  printf("=> Starting items: ");
  for (i = 0; i < monkey->n_items; i++) {
    printf("%ld, ",monkey->items[i]);
  }
  printf("\n");
  
  printf("=> Operation: new = %d%s %c %d%s\n",
         monkey->left_operand,
         monkey->left_operand_old ? "ld":"",
         monkey->operation,
         monkey->right_operand,
         monkey->right_operand_old ? "ld":"");
  
  printf("=> Test: %s by %d\n", 
         monkey->test_operation,
         monkey->test_operand);

  printf("=> If true: throw to monkey %d\n", 
         monkey->test_true_monkey);

  printf("=> If false: throw to monkey %d\n", 
         monkey->test_false_monkey);
  
  printf("\n");
}
