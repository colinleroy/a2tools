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

int read_from = ROM;
int write_to  = NONE;
int lc_bank   = 1;

struct _call_count {
  unsigned long call_count;
  unsigned long instruction_count;
  int called_from;
};

static int *call_tree = NULL;
static int tree_depth = 0;
static call_count **calls = NULL;

int analyze_instruction(int op_addr, char *instr, int param_addr, char *comment) {
  static int bit_count = 0;
  static int last_bit = 0;
  int parsed = 0, bank_switch = 0;

  comment[0] = '\0';

  if (!strcmp(instr, "bit")) {
    switch(param_addr) {
      case 0xC080:
        read_from = RAM;
        write_to  = NONE;
        lc_bank   = 2;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC081:
        if (param_addr == last_bit && ++bit_count == 2) {
          read_from = ROM;
          write_to  = RAM;
          lc_bank   = 2;
          parsed    = 1;
          bank_switch = 1;
        } else {
          bit_count = 0;
        }
        break;
      case 0xC082:
        read_from = ROM;
        write_to  = NONE;
        lc_bank   = 2;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC083:
        if (param_addr == last_bit && ++bit_count == 2) {
          read_from = RAM;
          write_to  = RAM;
          lc_bank   = 2;
          parsed    = 1;
          bank_switch = 1;
        } else {
          bit_count = 0;
        }
        break;
      case 0xC088:
        read_from = RAM;
        write_to  = NONE;
        lc_bank   = 1;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC089:
        if (param_addr == last_bit && ++bit_count == 2) {
          read_from = ROM;
          write_to  = RAM;
          lc_bank   = 1;
          parsed    = 1;
          bank_switch = 1;
        }
        break;
      case 0xC08A:
        read_from = ROM;
        write_to  = NONE;
        lc_bank   = 1;
        parsed    = 1;
        bank_switch = 1;
        break;
      case 0xC08B:
        if (param_addr == last_bit && ++bit_count == 2) {
          read_from = RAM;
          write_to  = RAM;
          lc_bank   = 1;
          parsed    = 1;
          bank_switch = 1;
        }
        break;
      default: break;
    }
    last_bit = param_addr;
    bit_count++;
  } else {
    bit_count = 0;
    last_bit = 0;
  }
  if (bank_switch) {
    snprintf(comment, BUF_SIZE,
            " ; BANK SWITCH: read from %s, write to %s, LC bank %d",
            mem_str[read_from], mem_str[write_to], lc_bank);
  }
  return parsed;
}

int is_instruction_write(const char *instr) {
  int i;
  for (i = 0; write_instructions[i] != NULL; i++) {
    if (!strcmp(write_instructions[i], instr)) {
      return 1;
    }
  }
  return 0;
}

static void count_call(void) {
  if (tree_depth > 0) {
    call_count *counter = calls[call_tree[tree_depth - 1]];
    int caller_addr = 0;
    if (tree_depth > 1)
      caller_addr = call_tree[tree_depth - 2];
    
    if (counter == NULL) {
      counter = malloc(sizeof(call_count));
      calls[call_tree[tree_depth - 1]] = counter;
      counter->call_count = 0;
      counter->instruction_count = 0;
    }
    counter->call_count++;
  }
}

static void count_instruction(void) {
  if (tree_depth > 0) {
    call_count *counter = calls[call_tree[tree_depth - 1]];
    int caller_addr = 0;
    if (tree_depth > 1)
      caller_addr = call_tree[tree_depth - 2];
    
    if (counter == NULL) {
      counter = malloc(sizeof(call_count));
      calls[call_tree[tree_depth - 1]] = counter;
      counter->call_count = 0;
      counter->instruction_count = 0;
    }
    counter->instruction_count++;
  }
}

void update_call_counters(const char *instr, int param_addr) {
  if (!strcmp(instr, "jsr")) {
    call_tree[tree_depth] = param_addr;
    tree_depth++;
    count_call();
  } else if (!strcmp(instr, "rts")) {
    tree_depth--;
  }

  count_instruction();
}

void print_counters(void) {
  int i;
  printf("; address; symbol; calls; instructions\n");
  for (i = 0; i < STORAGE_SIZE; i++) {
    if (calls[i]) {
      call_count *counter = calls[i];
      dbg_symbol *sym = symbol_get_by_addr(i);
      if (sym && symbol_get_name(sym))
        printf("; %04X; %s; %ld; %ld\n", i, 
          symbol_get_name(sym), counter->call_count, counter->instruction_count);
    }
  }
}

void allocate_trace_counters(void) {
  if (call_tree != NULL) {
    return;
  }
  call_tree  = malloc(sizeof(int) * STORAGE_SIZE);
  calls      = malloc(sizeof(call_count *) * STORAGE_SIZE);

  memset(call_tree, 0, sizeof(int) * STORAGE_SIZE);
  memset(calls,     0, sizeof(call_count *) * STORAGE_SIZE);
}

int is_lc_user_page_on(void) {
  return lc_bank == 2;
}

int is_addr_in_cc65_user_bank (int op_addr) {
  return (read_from == RAM && (op_addr < 0xD000 || op_addr >= 0xDFFF))
   || (read_from == RAM && op_addr >= 0xD000 && op_addr < 0xDFFF && is_lc_user_page_on());
}

int get_ram_write_destination(void) {
  return write_to;
}
int get_ram_read_source(void) {
  return read_from;
}
