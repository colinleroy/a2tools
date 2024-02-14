#ifndef __instructions_h
#define __instructions_h
#include "symbols.h"

typedef struct _function_calls function_calls;
typedef struct _instr_cycles instr_cycles;

#define RAM 0
#define ROM 1
#define NONE 2

#define CPU_6502  0
#define CPU_65816 1
/* Addresses we handle differently */
#define __MAIN_START__         start_addr
extern int ROM_IRQ_ADDR[];
extern int PRODOS_IRQ_ADDR;
#define PRODOS_MLI_RETURN_ADDR 0xBFB6

#define ADDR_MODE_IMMEDIATE      0  /* ADC #00       */
#define ADDR_MODE_IMPLIED        1  /* INY */
#define ADDR_MODE_ACC            2  /* ASL A         */
#define ADDR_MODE_ZERO           3  /* ADC $40       */
#define ADDR_MODE_ZEROIND        4  /* STA ($40)     */
#define ADDR_MODE_ZEROXY         5  /* ADC $40,x     */
#define ADDR_MODE_ABS            6  /* ADC $C080     */
#define ADDR_MODE_ABSXY          7  /* ADC $CO80,x   */
#define ADDR_MODE_ZINDX          8  /* ADC ($1234,x) */
#define ADDR_MODE_ZINDY          9  /* ADC ($1234),y */
#define ADDR_MODE_INDX          10  /* JMP ($1234,x) */
#define NUM_ADDR_MODES          11

#define FLAG_N 0b10000000
#define FLAG_V 0b01000000
#define FLAG_D 0b00001000
#define FLAG_I 0b00000100
#define FLAG_Z 0b00000010
#define FLAG_C 0b00000001

void allocate_trace_counters(void);
void start_tracing(int cpu);
int update_call_counters(int cpu, int op_addr, const char *instr, int param_addr, int cycle_count, int line_num);

int is_instruction_write(const char *instr);
int analyze_instruction(int cpu, int op_addr, const char *instr, int param_addr, char *comment);

int instruction_get_addressing_mode(int cpu, const char *instr, const char *arg);
int get_cycles_for_instr(int cpu, const char *instr, int a_mode, int flags);

/* Memory banking */
int is_addr_in_cc65_user_bank (int op_addr);

void finalize_call_counters(void);

#endif
