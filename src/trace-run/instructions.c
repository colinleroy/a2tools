#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "general.h"
#include "instructions.h"
#include "symbols.h"

#define STORAGE_SIZE 200000

char *mem_str[3] = {
  "RAM",
  "ROM",
  "none"
};

/* LSR, ROL, ROR, ? */

char *read_instructions[] = {
  "adc", "and", "bit",
  "cmp", "cpx", "cpy",
  "eor", "jmp", "jsr",
  "ora",
  "lda", "ldx", "ldy",
  "sbc",
  NULL
};

char *write_instructions[] = {
  "dec", "inc",
  "lsr", "rol", "ror",
  "sta", "stx", "sty",
  "stz", "trb", "tsb",
  NULL
};

struct _instr_cycles {
  char *instruction;
  int cost[NUM_ADDR_MODES];
};

/* Values are for the 65C02.
 * Ref1: https://www.masswerk.at/6502/6502_instruction_set.html
 * Ref2: http://6502.org/tutorials/65c02opcodes.html
 */
instr_cycles instr_cost[] = {
/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, indx, indy}} */
/* Math */
  {"adc", {2,   0,   0,   3,    5,      4,      4,   4,     6,    5}},
  {"and", {2,   0,   0,   3,    5,      4,      4,   4,     6,    5}},
  {"asl", {0,   0,   2,   5,    0,      6,      6,   6,     0,    0}},
  {"eor", {2,   0,   0,   3,    5,      4,      4,   4,     6,    5}},
  {"bit", {2,   0,   0,   3,    0,      4,      4,   4,     0,    0}},
  {"lsr", {0,   0,   2,   5,    0,      6,      6,   6,     0,    0}},
  {"ora", {2,   0,   0,   3,    5,      4,      4,   4,     6,    5}},
  {"rol", {0,   0,   2,   5,    0,      6,      6,   6,     0,    0}},
  {"ror", {0,   0,   2,   5,    0,      6,      6,   6,     0,    0}},
  {"sbc", {2,   0,   0,   3,    5,      4,      4,   4,     6,    5}},
  {"trb", {0,   0,   0,   5,    0,      0,      6,   0,     0,    0}},
  {"tsb", {0,   0,   0,   5,    0,      0,      6,   0,     0,    0}},

/* Flow */
  {"bcc", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"bcs", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"beq", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"bmi", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"bne", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"bpl", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"bra", {0,   0,   0,   0,    0,      0,      3,   0,     0,    0}},
  {"brk", {0,   7,   0,   0,    0,      0,      4,   0,     0,    0}},
  {"bvc", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"bvs", {0,   0,   0,   0,    0,      0,      2,   0,     0,    0}},
  {"jmp", {0,   0,   0,   0,    0,      0,      3,   6,     5,    5}},
  {"jsr", {0,   0,   0,   0,    0,      0,      6,   0,     0,    0}},
  {"nop", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"rti", {0,   6,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"rts", {0,   6,   0,   0,    0,      0,      0,   0,     0,    0}},

/* Flags */
/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, indx, indy}} */
  {"clc", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"cld", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"cli", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"clv", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"sec", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"sed", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"sei", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},

/* Compare */
  {"cmp", {2,   0,   0,   3,    5,      4,      4,   4,     6,    5}},
  {"cpx", {2,   0,   0,   3,    0,      0,      4,   0,     0,    0}},
  {"cpy", {2,   0,   0,   3,    0,      0,      4,   0,     0,    0}},

/* Decrement */
  {"dec", {0,   0,   2,   5,    0,      6,      6,   7,     0,    0}},
  {"dex", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"dey", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},

/* Increment */
  {"inc", {0,   0,   2,   5,    0,      6,      6,   7,     0,    0}},
  {"inx", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"iny", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},

/* Loads and stores */
/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, indx, indy}} */
  {"lda", {2,   0,   0,   3,    5,      4,      4,   4,     6,    5}},
  {"ldx", {2,   0,   0,   3,    0,      4,      4,   4,     0,    0}},
  {"ldy", {2,   0,   0,   3,    0,      4,      4,   4,     0,    0}},
  {"sta", {0,   0,   0,   3,    5,      4,      4,   5,     6,    6}},
  {"stx", {0,   0,   0,   3,    0,      4,      4,   0,     0,    0}},
  {"sty", {0,   0,   0,   3,    0,      4,      4,   0,     0,    0}},
  {"stz", {0,   0,   0,   3,    0,      4,      4,   5,     0,    0}},
  {"tax", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"tay", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"tsx", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"txa", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"txs", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"tya", {0,   2,   0,   0,    0,      0,      0,   0,     0,    0}},

/* Stack */
  {"pha", {0,   3,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"php", {0,   3,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"phx", {0,   3,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"phy", {0,   3,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"pla", {0,   4,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"plp", {0,   4,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"plx", {0,   4,   0,   0,    0,      0,      0,   0,     0,    0}},
  {"ply", {0,   4,   0,   0,    0,      0,      0,   0,     0,    0}},

/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, indx, indy}} */
  {NULL,  {0,   0,   0,   0,    0,      0,      0,   0,     0,    0}}
};

/* Current memory banking */
int read_from = RAM;
int write_to  = NONE;
int lc_bank   = 1;

extern int start_addr;
static int _main_addr;
extern int verbose;

/* Statistics structure */
struct _function_calls {
  /* The address */
  int addr;

  /* The source location */
  const char *file;
  int line;

  /* The associated debug symbol */
  dbg_symbol *func_symbol;

  /* Number of times the function's been called */
  unsigned long call_count;

  /* Number of instructions and cycles inside this function, total */
  unsigned long self_instruction_count;
  unsigned long self_cycle_count;

  /* Number of instructions and cycles inside this function, current call */
  unsigned long cur_call_self_instruction_count;
  unsigned long cur_call_self_cycle_count;

  /* Number of instructions and cycles inside this function,
   * plus the ones it calls */
  unsigned long incl_instruction_count;
  unsigned long incl_cycle_count;

  /* List of functions called by this one */
  function_calls **callees;
  /* Number of functions called by this one */
  int n_callees;
};

/* The current call tree */
static function_calls **func_tree = NULL;

/* The current call tree during an IRQ */
static function_calls **irq_tree = NULL;

/* Pointer to which call tree to use */
static function_calls **tree_functions = NULL;

/* The current tree depth */
static int            tree_depth = 0;

/* Storage for the tree depth before entering
 * IRQ handlers */
static int tree_depth_before_intr[2];

/* Interrupt handler counter (0xC803 will call
 * ProDOS's 0xBFEB), so two handlers will run */
static int in_interrupt = 0;

/* Remember if we just rti'd out of IRQ */
static int just_out_of_irq = 0;

/* Array of functions we track */
static function_calls **functions = NULL;

/* The first call (at start address) */
static function_calls *root_call = NULL;

/* Helper for mapping addresses and symbols */
int is_addr_in_cc65_user_bank (int op_addr) {
  return (read_from == RAM && (op_addr < 0xD000 || op_addr > 0xDFFF))
   || (read_from == RAM && op_addr >= 0xD000 && op_addr < 0xDFFF && lc_bank == 2);
}

/* Figure out the addressing mode for the instruction */
int instruction_get_addressing_mode(const char *arg) {
  int len;

  /* no arg */
  if (!arg || arg[0] == '\0')
    return ADDR_MODE_IMPLIED;

  len = strlen(arg);

  /* A */
  if (len == 1)
    return ADDR_MODE_ACC;

  /* #$ff */
  if (arg[0] == '#')
    return ADDR_MODE_IMMEDIATE;

  /* $ff */
  if (len == 3 && arg[0] == '$')
    return ADDR_MODE_ZERO;

  /* <$90 or similar, unsure of page, let's use absolute */
  if (arg[0] == '<' || arg[0] == '>')
    return ADDR_MODE_ABS;

  /* MAME symbol like RDRAM or 80STOREON :( */
  if ((arg[0] >= 'A' && arg[0] <= 'Z') || (arg[0] >= 'a' && arg[0] <= 'z') || (arg[0] >= '0' && arg[0] <= '9'))
    return ADDR_MODE_ABS;

  if (len >= 5) {

    /* $ff, x */
    if (arg[0] == '$' && arg[3] == ',')
      return ADDR_MODE_ZEROXY;

    /* ($ff), y */
    if (arg[0] == '(' && arg[4] == ')' && len > 7 && (arg[7] == 'x' || arg[7] == 'y'))
      return ADDR_MODE_ZEROXY;

    /* ($ff) */
    if (arg[0] == '(' && arg[4] == ')')
      return ADDR_MODE_ZEROIND;
  
    /* $ffff */
    if (len == 5 && arg[0] == '$')
      return ADDR_MODE_ABS;

    /* $ffff, x */
    if (len >= 6 && arg[0] == '$' && arg[5] == ',')
      return ADDR_MODE_ABSXY;

    /* ($ffff, x) */
    if (len >= 6 && arg[0] == '(' && arg[6] == ',' && arg[8] == 'x')
      return ADDR_MODE_INDX;

    /* ($ff, x) */
    if (arg[0] == '(' && arg[4] == ',' && arg[6] == 'x')
      return ADDR_MODE_INDX;

    /* ($ffff), y */
    if (len >= 7 && arg[0] == '(' && arg[6] == ')')
      return ADDR_MODE_INDY;

    /* ($ff), y */
    if (arg[0] == '(' && arg[4] == ',' && arg[7] == 'x')
      return ADDR_MODE_INDX;


  }
  fprintf(stderr, "Error: unknown addressing mode for %s\n", arg);
  exit(1);
}

/* Check instruction for memory banking change */
int analyze_instruction(int op_addr, const char *instr, int param_addr, char *comment) {
  static int bit_count = 0;
  static int last_bit = 0;
  int parsed = 0, bank_switch = 0;

  comment[0] = '\0';

  /* Dirty hack: The IRQ handler at 0x803 finishes by resetting
   * the memory banking to how it was at C893: inc $c000, x
   * For all intents and purposes we will assume we're going
   * back to the standard cc65 setup, Read RAM, LC bank 2 */
  if (op_addr == 0xC893 && !strcmp(instr, "inc") && param_addr == 0xc000) {
    goto ramlc2;
  }


  if (!strcmp(instr, "bit") || !strncmp(instr,"ld", 2)
   || !strncmp(instr,"st", 2) || !strcmp(instr, "inc")) {
    if (is_instruction_write(instr)) {
      /* only one write required (?) */
      bit_count++;
      last_bit = param_addr;
    }
    switch(param_addr) {
      case 0xC080:
      case 0xC084:
ramlc2:
        read_from = RAM;
        write_to  = NONE;
        lc_bank   = 2;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC081:
      case 0xC085:
        if (param_addr == last_bit) {
          bit_count++;
        } else {
          bit_count = 0;
        }
        read_from = ROM;
        write_to  = (bit_count == 2) ? RAM : NONE;
        lc_bank   = 2;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC082:
      case 0xC086:
        read_from = ROM;
        write_to  = NONE;
        lc_bank   = 2;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC083:
      case 0xC087:
        if (param_addr == last_bit) {
          bit_count++;
        } else {
          bit_count = 0;
        }
        read_from = RAM;
        write_to  = (bit_count == 2) ? RAM : NONE;
        lc_bank   = 2;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC088:
      case 0xC08C:
        read_from = RAM;
        write_to  = NONE;
        lc_bank   = 1;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC089:
      case 0xC08D:
        if (param_addr == last_bit) {
          bit_count++;
        } else {
          bit_count = 0;
        }
        read_from = ROM;
        write_to  = (bit_count == 2) ? RAM : NONE;
        lc_bank   = 1;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC08A:
      case 0xC08E:
        read_from = ROM;
        write_to  = NONE;
        lc_bank   = 1;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC08B:
      case 0xC08F:
        if (param_addr == last_bit) {
          bit_count++;
        } else {
          bit_count = 0;
        }
        read_from = RAM;
        write_to  = (bit_count == 2) ? RAM : NONE;
        lc_bank   = 1;
        parsed    = 1;
        bank_switch = 1;
        break;
      default:
        break;
    }
    last_bit = param_addr;
    bit_count++;
  } else {
    bit_count = 0;
    last_bit = 0;
  }

  /* Update comment for the caller */
  if (bank_switch) {
    snprintf(comment, BUF_SIZE,
            " ; BANK SWITCH: read from %s, write to %s, LC bank %d",
            mem_str[read_from], mem_str[write_to], lc_bank);
  }
  return parsed;
}

/* Returns 1 if the instruction is a "write" instruction */
int is_instruction_write(const char *instr) {
  int i;
  for (i = 0; write_instructions[i] != NULL; i++) {
    if (!strcmp(write_instructions[i], instr)) {
      return 1;
    }
  }
  return 0;
}

/* Alloc a stat struct */
static function_calls *function_calls_new(int addr) {
    function_calls *counter = malloc(sizeof(function_calls));
    memset(counter, 0, sizeof(function_calls));
    counter->addr = addr;

    return counter;
}

/* Stat struct getter. Will return the existing one, or
 * create and register a new one as needed. */
static function_calls *get_function_calls_for_addr(int addr) {
    function_calls *counter;
    dbg_slocdef *sloc_info = NULL;

    /* We want to register the first one away from
     * the array, so as to dump it first and once
     * in the
     * callgrind file */
    if (addr == start_addr) {
      if (!root_call)
        root_call = function_calls_new(start_addr);
      return root_call;
    }

    counter = functions[addr];

    /* Initialize the struct */
    if (counter == NULL) {
      counter = function_calls_new(addr);
      functions[addr] = counter;

      sloc_info = sloc_get_for_addr(addr);
      if (sloc_info) {
        counter->file        = sloc_get_filename(sloc_info);
        counter->line        = sloc_get_line(sloc_info);
      }
    }
    return counter;
}

/* Get cycles number for instruction / addressing mode
 * TODO Hugely inefficient, add a instruction str <=> number mapping
 */
int get_cycles_for_instr(const char *instr, int a_mode) {
  for (int i = 0; instr_cost[i].instruction != NULL; i++) {
    if (!strcmp(instr, instr_cost[i].instruction)) {
      if (instr_cost[i].cost[a_mode] == 0) {
        fprintf(stderr, "Error, addressing mode %d invalid for %s\n", a_mode, instr);
        return -1;
      }
      return instr_cost[i].cost[a_mode];
    }
  }
  fprintf(stderr, "Error, cycle count not found for %s\n", instr);
  return -1;
}

/* Count current instruction */
static void count_instruction(const char *instr, int cycle_count) {
  tree_functions[tree_depth - 1]->cur_call_self_instruction_count++;
  tree_functions[tree_depth - 1]->cur_call_self_cycle_count += cycle_count;
}

/* Find a function in an array */
static function_calls *find_func_in_list(function_calls *func, function_calls **list, int list_len) {
  int i;
  for (i = 0; i < list_len; i++) {
    function_calls *cur = list[i];
    if (cur->addr == func->addr) {
      return cur;
    }
  }
  return NULL;
}

/* Add a callee to a function */
static void add_callee(function_calls *caller, function_calls *callee) {
  function_calls *existing_callee = find_func_in_list(callee, caller->callees, caller->n_callees);

  if (!existing_callee) {
    /* First time caller calls callee. Init a new struct */
    existing_callee = function_calls_new(callee->addr);
    existing_callee->func_symbol = callee->func_symbol;
    existing_callee->file    = callee->file;
    existing_callee->line    = callee->line;

    /* Register new callee in caller's struct */
    caller->n_callees++;
    caller->callees = realloc(caller->callees, caller->n_callees * sizeof(function_calls *));
    caller->callees[caller->n_callees - 1] = existing_callee;
  }

  /* Increment the call count */
  existing_callee->call_count++;

  // if (verbose)
  //  fprintf(stderr, "; updated callee %04X %s count %ld to parent addr %04X, func %s\n",
  //         existing_callee->addr, symbol_get_name(existing_callee->func_symbol),
  //         existing_callee->call_count,
  //         caller->addr, symbol_get_name(caller->func_symbol));
}

/* Transfer the instruction count to the top of the tree */
static void update_caller_incl_cost(int instr_count, int cycle_count) {
  int i = tree_depth;
  for (i = tree_depth; i > 0; i--) {
    function_calls *caller = tree_functions[i - 1];
    function_calls *ref_callee = find_func_in_list(tree_functions[i], caller->callees, caller->n_callees);
    // if (verbose)
    //  fprintf(stderr, "; adding %d incl count to %s called by %s\n",
    //         count, symbol_get_name(callee->func_symbol), symbol_get_name(caller->func_symbol));
    ref_callee->incl_instruction_count += instr_count;
    ref_callee->incl_cycle_count += cycle_count;
  }
  /* and the root */
  tree_functions[0]->incl_instruction_count += instr_count;
  tree_functions[0]->incl_cycle_count += cycle_count;
}

/* Register a new call in the tree */
static void start_call_info(int addr, int mem, int lc, int line_num) {
  function_calls *my_info;

  if (tree_depth < 0) {
    fprintf(stderr, "Error, tree depth is negative (%d)\n", tree_depth);
    exit(1);
  }

  /* Get the stats structure */
  my_info = get_function_calls_for_addr(addr);
  /* FIXME shouldn't the stats structs be indexed by addr / mem / lc... */
  my_info->func_symbol = symbol_get_by_addr(addr, mem, lc);

  /* Log */
  if (verbose) {
    fprintf(stderr, ";");
    for (int i = 0; i < tree_depth; i++)
      fprintf(stderr, " ");

    fprintf(stderr, "depth %d, $%04X (%s/LC%d), %s calls %s (asm line %d)\n",
      tree_depth, addr, mem == RAM ? "RAM":"ROM", lc,
      tree_depth > 0 ? symbol_get_name(tree_functions[tree_depth - 1]->func_symbol) : "*start*",
      symbol_get_name(my_info->func_symbol), line_num);
  }

  /* Register in the call tree */
  tree_functions[tree_depth] = my_info;
  /* Reset current self instruction count for this new call */
  tree_functions[tree_depth]->cur_call_self_instruction_count = 0;
  tree_functions[tree_depth]->cur_call_self_cycle_count = 0;

  /* add myself to parent's callees */
  if (tree_depth > 0) {
    function_calls *parent_info = tree_functions[tree_depth - 1];
    add_callee(parent_info, my_info);
  }

  /* increment depth */
  tree_depth++;
}

static void end_call_info(int line_num) {
  function_calls *my_info;

  if (tree_depth == 0) {
    fprintf(stderr, "Can not go up call tree from depth 0\n");
    exit(1);
  }

  /* Go back up */
  tree_depth--;

  /* Get stats */
  my_info = tree_functions[tree_depth];

  /* Log */
  if (verbose) {
    fprintf(stderr, ";");
    for (int i = 0; i < tree_depth; i++)
      fprintf(stderr, " ");
    fprintf(stderr, "depth %d, %s returning (asm line %d)\n", tree_depth,
            symbol_get_name(my_info->func_symbol), line_num);
  }

  /* We done ? */
  if (tree_depth > 0) {
    /* update self counts */
    tree_functions[tree_depth]->self_instruction_count +=
      tree_functions[tree_depth]->cur_call_self_instruction_count;

    tree_functions[tree_depth]->self_cycle_count +=
      tree_functions[tree_depth]->cur_call_self_cycle_count;

    /* push it up to callers */
    update_caller_incl_cost(
      tree_functions[tree_depth]->cur_call_self_instruction_count,
      tree_functions[tree_depth]->cur_call_self_cycle_count);
  }
}

int update_call_counters(int op_addr, const char *instr, int param_addr, int cycle_count, int line_num) {
  int dest = RAM, lc = 0;

  /* Set memory and LC bank according to the address */
  if (param_addr < 0xD000 || param_addr > 0xDFFF) {
    dest = RAM;
    lc = 1;
  } else {
    dest = is_instruction_write(instr) ? write_to : read_from;
    lc = lc_bank;
  }

  /* Are we entering an IRQ handler ? */
  if (op_addr == ROM_IRQ_ADDR || op_addr == PRODOS_IRQ_ADDR) {
    if (verbose)
      fprintf(stderr, "; interrupt at depth %d\n", tree_depth);

    /* Record existing depth */
    tree_depth_before_intr[in_interrupt] = tree_depth;

    /* Increment the IRQ stack count */
    in_interrupt++;

    /* Update stats structs pointer */
    tree_functions = irq_tree;

    /* Reset tree depth */
    tree_depth = 0;

    /* And record entering */
    start_call_info(op_addr, RAM, lc, line_num);

  } else if (!strcmp(instr, "rti")) {
    /* Are we exiting an IRQ handler ? */
    if (in_interrupt) {
      if (verbose)
        fprintf(stderr, "; rti from interrupt at depth %d\n", tree_depth);

      /* Decrease IRQ stack */
      in_interrupt--;

      /* Record end of call */
      end_call_info(line_num);

      /* Reset tree to standard runtime */
      tree_functions = func_tree;
      tree_depth = tree_depth_before_intr[in_interrupt];

      /* Remember if we just rti'd out of IRQ, as if
       * we go back to a jsr that was recorded but not
       * executed, we don't want to re-register it. If
       * so we'll skip the next instruction. */
      just_out_of_irq = 1;

    } else if (op_addr == PRODOS_MLI_RETURN_ADDR) {
      /* Hack: ProDOS MLI calls return with an rti,
       * handle it as an rts. */
      goto rts;
    }

  /* cc65 runtime jmp's into _main, record it as if it
   * was a jsr */
  } else if (param_addr == _main_addr && !strcmp(instr, "jmp")) {
    goto jsr;

  /* Are we entering a function ? */
  } else if (!strcmp(instr, "jsr")) {
jsr:
    /* don't register a jsr if we just went out of
     * irq, we registered it right before entering IRQ */
    if (!just_out_of_irq)
      start_call_info(param_addr, dest, lc, line_num);

    /* Unset flag */
    just_out_of_irq = 0;

  /* Are we returning from a function? */
  } else if (!strcmp(instr, "rts") && tree_depth > 0) {
rts:
    /* don't register an rts if we just went out of irq,
     * we registered it before, same as jsr's */
    if (!just_out_of_irq)
      end_call_info(line_num);

    /* Unset flag */
    just_out_of_irq = 0;

  } else {
    /* At least one instruction since getting
     * out of IRQ, unset flag */
    just_out_of_irq = 0;
  }

  /* And count the instruction */
  count_instruction(instr, cycle_count);
  return 0;
}

/* Dump data in Callgrind format: callee information
 * this will be included for all functions.
 * Ref: https://valgrind.org/docs/manual/cl-format.html
 */
static void dump_callee(function_calls *cur) {
  dbg_symbol *sym = cur->func_symbol;

  /* Dump callee information (call count and
   * inclusive instruction count) */
  printf("cfi=%s%s\n"
         "cfn=%s\n"
         "calls=%ld\n"
         "%d %lu %lu\n",
          cur->file ? cur->file : "unknown.",
          cur->file ? "": symbol_get_name(sym),
          symbol_get_name(sym),
          cur->call_count,
          cur->line,
          cur->incl_instruction_count,
          cur->incl_cycle_count
        );
}

/* Dump data in Callgrind format: function information
 * this will be included for all functions.
 * Ref: https://valgrind.org/docs/manual/cl-format.html
 */
static void dump_function(function_calls *cur) {
  dbg_symbol *sym = cur->func_symbol;
  int i;

  /* Dump the function definition and self
   * instruction count */
  printf("fl=%s%s\n"
         "fn=%s\n"
         "%d %lu %lu\n",
          cur->file ? cur->file : "unknown.",
          cur->file ? "": symbol_get_name(sym),
          symbol_get_name(sym),
          cur->line,
          cur->self_instruction_count,
          cur->self_cycle_count
        );

  /* Dump information for every function it calls */
  for (i = 0; i < cur->n_callees; i++) {
    dump_callee(cur->callees[i]);
  }

  /* Separate for clarity. */
  printf("\n");
}

/* Everything is done, dump the callgrind
 * file. */
void finalize_call_counters(void) {
  int i;

  /* finish all calls to count their self count */
  while (tree_depth > 0)
    end_call_info(0);

  /* File header */
  printf("# callgrind format\n"
         "events: Instructions Cycles\n\n");

  /* Dump start function first */
  dump_function(root_call);

  /* And then all the rest. */
  for (i = 0; i < STORAGE_SIZE; i++) {
    if (functions[i]) {
      dump_function(functions[i]);
    }
  }
}

void start_tracing(void) {
    /* make ourselves a root in the IRQ call tree */
    tree_functions = irq_tree;
    start_call_info(start_addr, 0, RAM, 0);

    /* Hack - reset depth for the runtime root */
    tree_depth = 0;

    /* make ourselves a root in the runtime call tree */
    tree_functions = func_tree;
    start_call_info(start_addr, 0, RAM, 0);
}

/* Setup structures and data */
void allocate_trace_counters(void) {
  if (functions != NULL) {
    /* Already done */
    return;
  }

  functions        = malloc(sizeof(function_calls *) * 2 * STORAGE_SIZE);
  func_tree        = malloc(sizeof(function_calls *) * 2 * STORAGE_SIZE);
  irq_tree         = malloc(sizeof(function_calls *) * 2 * STORAGE_SIZE);

  memset(functions,        0, sizeof(function_calls *) * 2 * STORAGE_SIZE);
  memset(func_tree,        0, sizeof(function_calls *) * 2 * STORAGE_SIZE);
  memset(irq_tree,         0, sizeof(function_calls *) * 2 * STORAGE_SIZE);

  /* Let the depth where it is, so every call will
   * be attached to the start_addr function */

  /* register _main's address for later - crt0 will
   * jmp to it and we'll still want to record it. */
  _main_addr = symbol_get_addr(symbol_get_by_name("_main"));
}
