#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "general.h"
#include "instructions.h"
#include "symbols.h"

int ROM_IRQ_ADDR[] = {0xC803, 0xC074 };
int PRODOS_IRQ_ADDR = 0xBFEB;

int log_stack_to_stdout = 0;

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
  "dec", "inc", "asl",
  "lsr", "rol", "ror",
  "sta", "stx", "sty",
  "stz", "trb", "tsb",
  NULL
};

char *condbranch_instructions[] = {
  "bcc", "bcs", "beq",
  "bmi", "bne", "bpl",
  "bvc", "bvs",
  NULL
};

struct _instr_cycles {
  char *instruction;
  int cost[NUM_ADDR_MODES];
  int cost_if_taken;
};

/* Values are for the 65C02.
 * Ref1: https://www.masswerk.at/6502/6502_instruction_set.html
 * Ref2: http://6502.org/tutorials/65c02opcodes.html
 */
instr_cycles instr_cost[] = {
/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, zindx, zindy, indx}} */
/* Math */
  {"adc", {2,   0,   0,   3,    5,      4,      4,   4,     6,     5,     0}, 0},
  {"and", {2,   0,   0,   3,    5,      4,      4,   4,     6,     5,     0}, 0},
  {"asl", {0,   0,   2,   5,    0,      6,      6,   6,     0,     0,     0}, 0},
  {"eor", {2,   0,   0,   3,    5,      4,      4,   4,     6,     5,     0}, 0},
  {"bit", {2,   0,   0,   3,    0,      4,      4,   4,     0,     0,     0}, 0},
  {"lsr", {0,   0,   2,   5,    0,      6,      6,   6,     0,     0,     0}, 0},
  {"ora", {2,   0,   0,   3,    5,      4,      4,   4,     6,     5,     0}, 0},
  {"rol", {0,   0,   2,   5,    0,      6,      6,   6,     0,     0,     0}, 0},
  {"ror", {0,   0,   2,   5,    0,      6,      6,   6,     0,     0,     0}, 0},
  {"sbc", {2,   0,   0,   3,    5,      4,      4,   4,     6,     5,     0}, 0},
  {"trb", {0,   0,   0,   5,    0,      0,      6,   0,     0,     0,     0}, 0},
  {"tsb", {0,   0,   0,   5,    0,      0,      6,   0,     0,     0,     0}, 0},

/* Flow */
  {"bcc", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"bcs", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"beq", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"bmi", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"bne", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"bpl", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"bvc", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"bvs", {0,   0,   0,   0,    0,      0,      2,   0,     0,     0,     0}, 1},
  {"bra", {0,   0,   0,   0,    0,      0,      3,   0,     0,     0,     0}, 0},
  {"brk", {0,   7,   0,   0,    0,      0,      4,   0,     0,     0,     0}, 0},
  {"jmp", {0,   0,   0,   0,    0,      0,      3,   6,     0,     0,     6}, 0},
  {"jsr", {0,   0,   0,   0,    0,      0,      6,   0,     0,     0,     0}, 0},
  {"nop", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"rti", {0,   6,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"rts", {0,   6,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},

/* Flags */
/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, zindx, zindy}} */
  {"clc", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"cld", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"cli", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"clv", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"sec", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"sed", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"sei", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},

/* Compare */
  {"cmp", {2,   0,   0,   3,    5,      4,      4,   4,     6,     5,     0}, 0},
  {"cpx", {2,   0,   0,   3,    0,      0,      4,   0,     0,     0,     0}, 0},
  {"cpy", {2,   0,   0,   3,    0,      0,      4,   0,     0,     0,     0}, 0},

/* Decrement */
  {"dec", {0,   0,   2,   5,    0,      6,      6,   7,     0,     0,     0}, 0},
  {"dex", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"dey", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},

/* Increment */
  {"inc", {0,   0,   2,   5,    0,      6,      6,   7,     0,     0,     0}, 0},
  {"inx", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"iny", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},

/* Loads and stores */
/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, zindx, zindy}} */
  {"lda", {2,   0,   0,   3,    5,      4,      4,   4,     6,     5,     0}, 0},
  {"ldx", {2,   0,   0,   3,    0,      4,      4,   4,     0,     0,     0}, 0},
  {"ldy", {2,   0,   0,   3,    0,      4,      4,   4,     0,     0,     0}, 0},
  {"sta", {0,   0,   0,   3,    5,      4,      4,   5,     6,     6,     0}, 0},
  {"stx", {0,   0,   0,   3,    0,      4,      4,   0,     0,     0,     0}, 0},
  {"sty", {0,   0,   0,   3,    0,      4,      4,   0,     0,     0,     0}, 0},
  {"stz", {0,   0,   0,   3,    0,      4,      4,   5,     0,     0,     0}, 0},
  {"tax", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"tay", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"tsx", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"txa", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"txs", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"tya", {0,   2,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},

/* Stack */
  {"pha", {0,   3,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"php", {0,   3,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"phx", {0,   3,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"phy", {0,   3,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"pla", {0,   4,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"plp", {0,   4,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"plx", {0,   4,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},
  {"ply", {0,   4,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},

  {"kil", {0,   0,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0},

/* {name, {imm, imp, acc, zero, zeroind,zeroxy, abs, absxy, zindx, zindy}} */
  {NULL,  {0,   0,   0,   0,    0,      0,      0,   0,     0,     0,     0}, 0}
};

/* Current memory banking */
int read_from = RAM;
int write_to  = NONE;
int lc_bank   = 1;

extern int start_addr;
static int _main_addr;
static int handle_rom_irq_addr = 0xffffffff;
static int handle_ram_irq_addr = 0xffffffff;

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

  /* Stack state */
  int stack_bytes_pushed;
};

/* The current call tree */
static function_calls **func_tree = NULL;

/* The current call tree during an IRQ */
static function_calls **irq_tree = NULL;

/* Pointer to which call tree to use */
static function_calls **tree_functions = NULL;

/* The current tree depth */
static int tree_depth = 0;

/* Storage for the tree depth before entering
 * IRQ handlers */
static int tree_depth_before_intr[2];

/* MLI hack (rti'ng from anywhere) */
static int mli_call_depth = -1;

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
  return op_addr < 0xD000 || op_addr > 0xDFFF || (read_from == RAM && lc_bank == 2);
}

/* Figure out the addressing mode for the instruction */
int instruction_get_addressing_mode(int cpu, const char *instr, const char *arg) {
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

  if (strstr(arg, ",s") - arg + 2 == strlen(arg)) {
    return ADDR_MODE_ABSXY; /* FIXME that's not true */
  }

  /* ($ff) */
  if (len == 5 && arg[0] == '(' && arg[1] == '$' && arg[4] == ')')
    return ADDR_MODE_ZEROIND;

  /* $ff, x */
  if ((len == 5 || len == 6) && arg[0] == '$' && arg[3] == ',')
    return ADDR_MODE_ZEROXY;

  /* ($ff), y */
  if (len == 8 && arg[0] == '(' && arg[4] == ')' && arg[7] == 'y')
    return ADDR_MODE_ZINDY;

  /* ($ff),y */
  if (len == 7 && arg[0] == '(' && arg[4] == ')' && arg[6] == 'y')
    return ADDR_MODE_ZINDY;

  /* $ffff */
  if (len == 5 && arg[0] == '$')
    return ADDR_MODE_ABS;

  /* $ffff, x */
  if ((len == 7 || len == 8) && arg[0] == '$' && arg[5] == ',')
    return ADDR_MODE_ABSXY;

  /* ($ffff) */
  if (len == 7 && arg[0] == '(' && arg[6] == ')')
    return ADDR_MODE_INDX;

  /* ($ffff, x) */
  if (len == 10 && arg[0] == '(' && arg[6] == ',' && arg[8] == 'x')
    return ADDR_MODE_INDX;

  /* ($ffff,x) */
  if (len == 9 && arg[0] == '(' && arg[6] == ',' && arg[7] == 'x')
    return ADDR_MODE_INDX;

  /* ($ff, x) */
  if (len == 8 && arg[0] == '(' && arg[4] == ',' && arg[6] == 'x')
    return ADDR_MODE_ZINDX;

  /* ($ff,x) */
  if (len == 7 && arg[0] == '(' && arg[4] == ',' && arg[5] == 'x')
    return ADDR_MODE_ZINDX;

  /* $ffffff */
  if (len == 7 && arg[0] == '$')
    return ADDR_MODE_ABS;

  /* $ffffff,x */
  if (len == 9 && arg[0] == '$' && arg[7]==',')
    return ADDR_MODE_ABSXY;

  fprintf(stderr, "Warning: unknown addressing mode for %s %s\n", instr, arg);
  return ADDR_MODE_ABS;
}

extern int cur_65816_bank;

/* Check instruction for memory banking change */
int analyze_instruction(int cpu, int op_addr, const char *instr, int param_addr, char *comment) {
  static int bit_count = 0;
  static int last_bit = 0;
  static int last_rom_bank = 0;
  int parsed = 0, bank_switch = 0, rom_bank_switch = 0;

  if (cpu == CPU_65816) {
    if (cur_65816_bank == 0xFF) {
      read_from = ROM;
    } else {
      read_from = RAM;
    }
    return 0;
  }

  comment[0] = '\0';

  if (!strcmp(instr, "bit") || !strncmp(instr,"ld", 2)
   || !strncmp(instr,"st", 2) || !strcmp(instr, "inc")) {
    if (is_instruction_write(instr)) {
      /* only one write required (?) */
      bit_count++;
      last_bit = param_addr;
    }
    switch(param_addr) {
      case 0xC020:
      case 0xC021:
      case 0xC022:
      case 0xC023:
      case 0xC024:
      case 0xC025:
      case 0xC026:
      case 0xC027:
      case 0xC028:
      case 0xC029:
      case 0xC02A:
      case 0xC02B:
      case 0xC02C:
      case 0xC02D:
      case 0xC02E:
      case 0xC02F:
        rom_bank_switch = 1;
        last_rom_bank = !last_rom_bank;
        parsed = 1;
        break;
      case 0xC080:
      case 0xC084:
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
            "BANK SWITCH: read from %s, write to %s, LC bank %d",
            mem_str[read_from], mem_str[write_to], lc_bank);
  } else if (rom_bank_switch) {
    snprintf(comment, BUF_SIZE,
            "ROM BANK SWITCH: %s paged in",
            last_rom_bank ? "Alt":"Main");
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

int is_instruction_condbranch(const char *instr) {
  int i;
  for (i = 0; condbranch_instructions[i] != NULL; i++) {
    if (!strcmp(condbranch_instructions[i], instr)) {
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
int get_cycles_for_instr(int cpu, const char *instr, int a_mode, int flags) {
  for (int i = 0; instr_cost[i].instruction != NULL; i++) {
    if (!strcmp(instr, instr_cost[i].instruction)) {
      int cost;
      if (instr_cost[i].cost[a_mode] == 0) {
        fprintf(stderr, "Warning, addressing mode %d invalid for %s\n", a_mode, instr);
        return instr_cost[i].cost[ADDR_MODE_ABS];
      }
      cost = instr_cost[i].cost[a_mode];

      if (instr_cost[i].cost_if_taken) {
        if (!strcmp(instr, "beq") && (flags & FLAG_Z) != 0)
          cost += instr_cost[i].cost_if_taken;
        else if (!strcmp(instr, "bne") && (flags & FLAG_Z) == 0)
          cost += instr_cost[i].cost_if_taken;

        else if (!strcmp(instr, "bcs") && (flags & FLAG_C) != 0)
          cost += instr_cost[i].cost_if_taken;
        else if (!strcmp(instr, "bcc") && (flags & FLAG_C) == 0)
          cost += instr_cost[i].cost_if_taken;

        else if (!strcmp(instr, "bmi") && (flags & FLAG_N) != 0)
          cost += instr_cost[i].cost_if_taken;
        else if (!strcmp(instr, "bpl") && (flags & FLAG_N) == 0)
          cost += instr_cost[i].cost_if_taken;

        else if (!strcmp(instr, "bvs") && (flags & FLAG_V) != 0)
          cost += instr_cost[i].cost_if_taken;
        else if (!strcmp(instr, "bvc") && (flags & FLAG_V) == 0)
          cost += instr_cost[i].cost_if_taken;
      }
      return cost;
    }
  }
  if (cpu == CPU_6502) {
    fprintf(stderr, "Error, cycle count not found for %s\n", instr);
    return 0;
  } else {
    /* FIXME add 65816 instructions */
    return 0;
  }
}

/* Count current instruction */
static int count_instruction(const char *instr, int cycle_count) {
  if (tree_depth == 0)
    return -1;
  tree_functions[tree_depth - 1]->cur_call_self_instruction_count++;
  tree_functions[tree_depth - 1]->cur_call_self_cycle_count += cycle_count;
  return 0;
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

static void tabulate_stack(void) {
  fprintf(stderr, ";");
  for (int i = 0; i < tree_depth; i++)
    fprintf(stderr, " ");
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
static void start_call_info(int cpu, int addr, int mem, int lc, int line_num) {
  function_calls *my_info;

  if (tree_depth < 0) {
    fprintf(stderr, "Error, tree depth is negative (%d)\n", tree_depth);
    exit(1);
  }

  /* Get the stats structure */
  my_info = get_function_calls_for_addr(addr);
  /* FIXME shouldn't the stats structs be indexed by addr / mem / lc... */
  my_info->func_symbol = symbol_get_by_addr(cpu, addr, mem, lc);

  my_info->stack_bytes_pushed = 2; /* return address */

  if (my_info->func_symbol == NULL) {
    fprintf(stderr, "No symbol found for 0x%04X, %d, %d\n", addr, mem, lc);
    exit(1);
  }
  /* Log */
  if (verbose) {
    tabulate_stack();

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

static void end_call_info(int op_addr, int line_num, const char *from) {
  function_calls *my_info;

  if (tree_depth == 0) {
    fprintf(stderr, "Can not go up call tree from depth 0\n");
    exit(1);
  }

  /* Get stats */
  my_info = tree_functions[tree_depth-1];

  my_info->stack_bytes_pushed -= 2; /* pop return address */
  if (my_info->stack_bytes_pushed != 0) {
    if (verbose) {
      tabulate_stack();
      fprintf(stderr, "warning - stack imbalance (%d)\n", my_info->stack_bytes_pushed);
    } else if (log_stack_to_stdout) {
      printf("                                                                 ; *WARNING - stack imbalance (%d)\n", my_info->stack_bytes_pushed);
    }
  }
  while (my_info->stack_bytes_pushed < 0 && my_info->stack_bytes_pushed % 2 == 0) {
    if (verbose) {
      tabulate_stack();
      fprintf(stderr, "returning one extra level\n");
    } else if (log_stack_to_stdout) {
      printf("                                                                 ; *RETURNING one extra level\n");
    }
    tree_depth--;
    my_info->stack_bytes_pushed += 2;
  }

  /* Go back up */
  tree_depth--;

  /* Log */
  if (verbose) {
    tabulate_stack();
    fprintf(stderr, "depth %d, %s returning after %zu cycles, (%s, asm line %d)\n", tree_depth,
            symbol_get_name(my_info->func_symbol),
            tree_functions[tree_depth]->cur_call_self_cycle_count,
            from, line_num);
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

int update_call_counters(int cpu, int op_addr, const char *instr, int param_addr, int cycle_count, int line_num) {
  int dest = RAM, lc = 0;

  function_calls *my_info = tree_functions[tree_depth - 1];

  if (cpu == CPU_6502) {
    /* Set memory and LC bank according to the address */
    if (param_addr < 0xD000 || param_addr > 0xDFFF) {
      dest = RAM;
      lc = 1;
    } else {
      dest = is_instruction_write(instr) ? write_to : read_from;
      lc = lc_bank;
    }
  }

  /* Count the instruction */
  if (count_instruction(instr, cycle_count) != 0) {
    return -1;
  }

  if (!strncmp(instr, "ph", 2)) {
    my_info->stack_bytes_pushed++;
  } else if (!strncmp(instr, "pl", 2)) {
    my_info->stack_bytes_pushed--;
  }

  /* Are we entering an IRQ handler ? */
  if (op_addr == ROM_IRQ_ADDR[cpu] || op_addr == PRODOS_IRQ_ADDR
   || op_addr == handle_rom_irq_addr || op_addr == handle_ram_irq_addr) {
    if (verbose)
      fprintf(stderr, "; interrupt at depth %d\n", tree_depth);

    if (in_interrupt == 0) {
      /* Record existing depth */
      tree_depth_before_intr[in_interrupt] = tree_depth;

      /* Update stats structs pointer */
      tree_functions = irq_tree;

      /* Reset tree depth */
      tree_depth = 1;
    }

    /* Increment the IRQ stack count */
    in_interrupt++;

    /* And record entering */
    start_call_info(cpu, op_addr, RAM, lc, line_num);

  } else if (!strcmp(instr, "rti")) {
    /* Are we exiting an IRQ handler ? */
    if (in_interrupt) {
      if (verbose)
        fprintf(stderr, "; rti from interrupt at depth %d\n", tree_depth);

      /* Record end of call */
      end_call_info(op_addr, line_num, "irq");

      /* Decrease IRQ stack */
      in_interrupt--;

      if (!in_interrupt) {
        /* Reset tree to standard runtime */
        tree_functions = func_tree;
        tree_depth = tree_depth_before_intr[in_interrupt];

        /* Remember if we just rti'd out of IRQ, as if
         * we go back to a jsr that was recorded but not
         * executed, we don't want to re-register it. If
         * so we'll skip the next instruction. */
        just_out_of_irq = 1;
      }
    } else if (op_addr == PRODOS_MLI_RETURN_ADDR) {
      /* Hack: ProDOS MLI calls return with an rti,
       * handle it as an rts, and go back up to caller's depth. */
      if (mli_call_depth == -1) {
        fprintf(stderr, "Untracked end of MLI call at depth %d\n", tree_depth);
        exit(1);
      } else {
        if (verbose) {
          tabulate_stack();
          fprintf(stderr, "(End of MLI call at depth %d)\n", tree_depth);
        }
        /* don't register an rts if we just went out of irq,
         * we registered it before, same as jsr's */
        if (!just_out_of_irq) {
          while (tree_depth > mli_call_depth)
            end_call_info(op_addr, line_num, "mli rts");
        }
        if (verbose) {
          tabulate_stack();
          fprintf(stderr, "(Returned from MLI call at depth %d)\n", tree_depth);
        }

        /* Unset flag */
        just_out_of_irq = 0;

        /* Unset MLI call flag */
        mli_call_depth = -1;
      }
    }

  /* cc65 runtime jmp's into _main, record it as if it
   * was a jsr */
  } else if (param_addr == _main_addr && !strcmp(instr, "jmp")) {
    goto jsr;

  /* Are we entering a function ? */
} else if (!strcmp(instr, "jsr") || !strcmp(instr, "jsl")) {
jsr:
    /* don't register a jsr if we just went out of
     * irq, we registered it right before entering IRQ */
    if (!just_out_of_irq) {
      start_call_info(cpu, param_addr, dest, lc, line_num);

      /* Ugly hack (both because of MAME's symbol and because of the hardcoding).
       * ProDOS MLI uses rti to return faster to where it was called from. */
      if (!strncmp(symbol_get_name(tree_functions[tree_depth - 1]->func_symbol), "RAM.LC1.ProDOS 8: ", 18)) {
        if (mli_call_depth != -1) {
          fprintf(stderr, "Error: unhandled recursive MLI call at line %d\n", line_num);
          //exit(1);
        }
        mli_call_depth = tree_depth - 1;
        if (verbose) {
          tabulate_stack();
          fprintf(stderr, "Recording MLI call at depth %d\n", mli_call_depth);
        }
      }
    }

    /* Unset flag */
    just_out_of_irq = 0;

  /* Are we returning from a function? */
} else if ((!strcmp(instr, "rts") || !strcmp(instr, "rtl")) && tree_depth > 0) {
    /* don't register an rts if we just went out of irq,
     * we registered it before, same as jsr's */
    if (!just_out_of_irq)
      end_call_info(op_addr, line_num, instr);

    /* Unset flag */
    just_out_of_irq = 0;

  } else {
    /* At least one instruction since getting
     * out of IRQ, unset flag */
    just_out_of_irq = 0;
  }

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
  dbg_symbol *sym;
  int i;

  if (cur == NULL) {
    return;
  }

  sym = cur->func_symbol;
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
    end_call_info(0x0000, 0, "end of program");

  /* File header */
  printf("# callgrind format\n"
         "events: Instructions Cycles\n\n");

  /* Dump start function first */
  dump_function(root_call);

  /* And then all the rest. */
  for (i = 0; i < 2*STORAGE_SIZE; i++) {
    if (functions[i]) {
      dump_function(functions[i]);
    }
  }
}

void start_tracing(int cpu) {
    /* make ourselves a root in the IRQ call tree */
    tree_functions = irq_tree;
    start_call_info(cpu, start_addr, RAM, 1, 0);

    /* Hack - reset depth for the runtime root */
    tree_depth = 0;

    /* make ourselves a root in the runtime call tree */
    tree_functions = func_tree;
    start_call_info(cpu, start_addr, RAM, 1, 0);
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
  _main_addr = symbol_get_addr(symbol_get_by_name("_main", NORMAL));

  if (symbol_get_by_name("handle_rom_irq", NORMAL))
    handle_rom_irq_addr = symbol_get_addr(symbol_get_by_name("handle_rom_irq", NORMAL));
  if (symbol_get_by_name("handle_ram_irq", NORMAL))
    handle_ram_irq_addr = symbol_get_addr(symbol_get_by_name("handle_ram_irq", NORMAL));
}
