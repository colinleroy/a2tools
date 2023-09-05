#ifndef __instructions_h
#define __instructions_h
#include "symbols.h"

typedef struct _function_calls function_calls;
typedef struct _instr_cycles instr_cycles;

#define RAM 0
#define ROM 1
#define NONE 2

/* Addresses we handle differently */
#define __MAIN_START__         start_addr
#define ROM_IRQ_ADDR           0xC803
#define PRODOS_IRQ_ADDR        0xBFEB
#define PRODOS_MLI_RETURN_ADDR 0xBFB6

#define ADDR_MODE_IMMEDIATE 0 /* ADC #00       */
#define ADDR_MODE_IMPLIED   1 /* INY */
#define ADDR_MODE_ACC       2 /* ASL A         */
#define ADDR_MODE_ZERO      3 /* ADC $40       */
#define ADDR_MODE_ZEROIND   4 /* STA ($40)     */
#define ADDR_MODE_ZEROXY    5 /* ADC $40,x     */
#define ADDR_MODE_ABS       6 /* ADC $C080     */
#define ADDR_MODE_ABSXY     7 /* ADC $CO80,x   */
#define ADDR_MODE_INDX      8 /* ADC ($1234,x) */
#define ADDR_MODE_INDY      9 /* ADC ($1234),y */

#define NUM_ADDR_MODES      10

void allocate_trace_counters(void);
void start_tracing(void);
int update_call_counters(int op_addr, const char *instr, int param_addr, int cycle_count, int line_num);

int is_instruction_write(const char *instr);
int analyze_instruction(int op_addr, const char *instr, int param_addr, char *comment);

int instruction_get_addressing_mode(const char *arg);
int get_cycles_for_instr(const char *instr, int a_mode, int *extra_cost_if_taken);

/* Memory banking */
int is_addr_in_cc65_user_bank (int op_addr);

void finalize_call_counters(void);

#endif
