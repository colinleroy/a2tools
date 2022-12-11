/* types */
typedef struct _monkey {
  long *items;
  int n_items;

  int left_operand_old;
  int left_operand;
  char operation;
  int right_operand_old;
  int right_operand;

  char *test_operation;
  int test_operand;
  
  int test_true_monkey;
  int test_false_monkey;
  
  int num_inspections;
} monkey;

/* Magic numbers */
enum {
  PARSE_MONKEY         = 0,
  PARSE_ITEMS          = 1,
  PARSE_OPERATION      = 2,
  PARSE_TEST_CONDITION = 3,
  PARSE_TEST_TRUE      = 4,
  PARSE_TEST_FALSE     = 5,
  PARSE_EMPTY          = 6,
  PARSE_DONE           = 7
};

#define ITEMS_START_TOKEN 2
#define OPERATION_LEFT_OPERAND_TOKEN 3 
#define OPERATION_TOKEN 4
#define OPERATION_RIGHT_OPERAND_TOKEN 5
#define TEST_OPERATION_TOKEN 1
#define TEST_OPERAND_TOKEN 3
#define DEST_MONKEY_TOKEN 5
