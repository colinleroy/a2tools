#ifndef __instructions_h
#define __instructions_h

typedef struct _call_count call_count;

#define RAM 0
#define ROM 1
#define NONE 2

void allocate_trace_counters(void);
void update_call_counters(const char *instr, int param_addr);

int is_instruction_write(const char *instr);
int analyze_instruction(int op_addr, char *instr, int param_addr, char *comment);

/* Memory banking */
int is_addr_in_cc65_user_bank (int op_addr);
int is_lc_user_page_on(void);
int get_ram_write_destination(void);
int get_ram_read_source(void);

void print_counters(void);

#endif
