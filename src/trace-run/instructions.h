#ifndef __instructions_h
#define __instructions_h
#include "symbols.h"

typedef struct _function_calls function_calls;
#define RAM 0
#define ROM 1
#define NONE 2

#define ROM_IRQ_ADDR           0xC803
#define PRODOS_IRQ_ADDR        0xBFEB

#define PRODOS_MLI_RETURN_ADDR 0xBFB6

void allocate_trace_counters(void);
int update_call_counters(int op_addr, const char *instr, int param_addr, int line_num);

int is_instruction_write(const char *instr);
int analyze_instruction(int op_addr, char *instr, int param_addr, char *comment);

/* Memory banking */
int is_addr_in_cc65_user_bank (int op_addr);

void finalize_call_counters(void);

#endif
