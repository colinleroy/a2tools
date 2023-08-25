#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "strsplit.h"
#include "slist.h"
#include "general.h"
#include "symbols.h"
#include "instructions.h"

#define FIELD_WIDTH 40
#define DEFAULT_START_ADDR 0x803

int tail = 0;
int verbose = 0;
int do_callgrind = 0;
int found_start_addr = 0;

int start_addr = DEFAULT_START_ADDR;

extern int read_from;
extern int write_to;
extern int lc_bank;

static int hex2int(const char *hex) {
  if (tolower(hex[1]) != 'x') {
    char zerox[32];
    snprintf(zerox, 32, "0x%s", hex);
    return strtoul(zerox, (char**)0, 0);
  } else {
    return strtoul(hex, (char**)0, 0);
  }
}

/* Remove the most annoying MAME symbols 
 * and put back the address instead 
 */
static char *fix_param(char *param) {
  if (!param)
    return NULL;
  if (!strcmp(param, "ROMIN"))
    return "$c085";
  if (!strcmp(param, "LCBANK2"))
    return "$c087";
  if (!strcmp(param, "($03fe)"))
    return "$befb";
  return param;
}

static void tabulate(const char *buf, int len) {
  int i;
  if (do_callgrind)
    return;

  for (i = buf ? strlen(buf) : 0; i < len; i++) {
      printf(" ");
  }
}
static void annotate_run(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];
  char line_buf[BUF_SIZE];
  int cur_line = 0;

  if (fp == NULL) {
    fprintf(stderr, "Can not open file %s: %s\n", file, strerror(errno));
    exit(1);
  }

skip_to_start:
  line_buf[0] = '\0';

  while (fgets(buf, BUF_SIZE, fp) || tail) {
    char **parts = NULL;
    char *line;
    int n_parts;
    int is_line;
    int buf_len;
    int a_mode;
    int cycles;

    cur_line++;
    buf_len = strlen(buf);

    if (buf_len == 0) {
      /* Wait for new data if tailing */
      if (tail) {
        clearerr(fp);
        usleep(5*1000);
        continue;
      } else {
        return;
      }
    }

    is_line = buf[buf_len - 1] == '\n';
    strcat(line_buf, buf);
    if (!is_line) {
      /* reset buf */
      buf[0] = '\0';
      /* Store until we get a new line */
      continue;
    }

    *strchr(line_buf, '\n') = '\0';

    /* we're going to poke holes in the string */
    line = strdup(line_buf);

    n_parts = strsplit_in_place(line, ':', &parts);

    /* We want at least an "ADDR: op" */
    if (n_parts >= 2) {
      int op_addr = 0;
      int param_addr = 0;
      dbg_slocdef *sloc = NULL;
      dbg_symbol *instr_symbol = NULL;
      dbg_symbol *param_symbol = NULL;
      char *instr, *arg;
      char comment[BUF_SIZE];

      /* get the op's address */
      op_addr = hex2int(parts[0]);

      /* If we've not yet seen the start address,
       * skip line
       */
      if (!found_start_addr) {
        if (op_addr == start_addr) {
          found_start_addr = 1;
          cur_line = 1;
        }
        else
          goto skip_to_start;
      }

      /* get instruction */
      instr = parts[1] + 1; /* skip space */

      /* get arg if there's one */
      arg = strchr(instr, ' ');
      if (arg) {
        *arg = '\0';
        arg++;
        arg = fix_param(arg);
      }

      if (op_addr >= 0) {
        /* Get our sloc only if the address is in RAM, out of LC or in LC page 2
         * Otherwise we're quite sure not to have a real symbol defined
         */
        if (is_addr_in_cc65_user_bank(op_addr)) {
          sloc = sloc_get_for_addr(op_addr);
        }
        instr_symbol = symbol_get_by_addr(op_addr, read_from, lc_bank);
        if (!instr_symbol) {
          instr_symbol = generate_symbol(parts[0], op_addr, read_from, lc_bank, NULL);
        }
      }

      /* get addressing mode and cycles count.
       * Done here instead of in instructions.c to be
       * able to display a potentially problematic line
       */
      a_mode = instruction_get_addressing_mode(arg);
      if ((cycles = get_cycles_for_instr(instr, a_mode)) < 0) {
        fprintf(stderr, "%s\n", buf);
        exit(1);
      }
      /* get param if it's numeric */
      if (arg && arg[0] == '$') {
        if (strchr(arg, '\n'))
          *strchr(arg, '\n') = '\0';
        if (strchr(arg, ','))
          *strchr(arg, ',') = '\0';

        param_addr = hex2int(arg + 1); /* skip $ */

        if (param_addr >= 0) {
          int dest = is_instruction_write(instr) ? write_to : read_from;
          param_symbol = symbol_get_by_addr(param_addr, dest, lc_bank);

          /* If we don't have a symbol, generate a dummy one */
          if (param_symbol == NULL)
            goto try_gen;
        } else {
          goto try_gen;
        }
      } else if (arg) {
try_gen:
        /* Generate a dummy symbol. Its name will reference its address,
         * and where it will hit depending on current memory banking 
         */
        int dest = is_instruction_write(instr) ? write_to : read_from;
        param_symbol = generate_symbol(arg, param_addr, dest, lc_bank, n_parts > 2 ? parts[2] : NULL);
        param_addr = symbol_get_addr(param_symbol);
      }

      /* Print the line as-is */
      if (!do_callgrind)
        printf("%s", line_buf);

      /* Profile if needed */
      if (do_callgrind && update_call_counters(op_addr, instr, param_addr, cycles, cur_line) < 0) {
        printf("; Error popping call tree at trace line %d\n", cur_line);
      }

      /* Analyse instruction to follow memory banking */
      if (analyze_instruction(op_addr, instr, param_addr, comment)
       || (!sloc && !instr_symbol && !param_symbol)) {
        /* print either banking comment or finish the line if we have zero data about it */
        if (!do_callgrind)
          printf("%s\n", comment);

      } else {
        if (!do_callgrind) {
          tabulate(line_buf, 18);
          printf("; OP: ");
        }
        /* Display the instruction's symbol */
        if (instr_symbol) {
          if (!do_callgrind) {
            printf("%s", symbol_get_name(instr_symbol));
            tabulate(symbol_get_name(instr_symbol), FIELD_WIDTH);
          }
        } else {
          tabulate(NULL, FIELD_WIDTH);
        }

        /* Print the argument's symbol */
        if (!do_callgrind)
          printf("ARG: ");
        if (param_symbol) {
          if (!do_callgrind) {
            printf("%s", symbol_get_name(param_symbol));
            tabulate(symbol_get_name(param_symbol), FIELD_WIDTH);
          }
        } else if (arg && arg[0] == '$') {
          if (!do_callgrind) {
            printf("%s", arg);
            tabulate(arg, FIELD_WIDTH);
          }
        } else {
          tabulate(NULL, FIELD_WIDTH);
        }

        /* Print the source location */
        if (sloc && !do_callgrind) {
          printf("OP loc: %s:%d", sloc_get_filename(sloc), sloc_get_line(sloc));
        }
        if (!do_callgrind) 
          printf("\n");
      }
    } else {
      /* Very probably a "(Loop for n instructions)" message from MAME */
      if (!do_callgrind) 
        printf("%s\n", line_buf);
      if (do_callgrind) {
        printf("Error: unexpected line :\n%s\n", line_buf);
        printf("Profiles must be generated from MAME with full traces:\n"
               "trace log.run,maincpu,noloop'\n");
        fclose(fp);
        return;
      }
    }

    free(line);
    free(parts);

    /* reset read buffers */
    line_buf[0] = '\0';
    buf[0] = '\0';
  }

  fclose(fp);

  /* Finish the counts */
  if (do_callgrind)
    finalize_call_counters();
}

int main(int argc, char *argv[]) {
  int i;
  char *trace_file = NULL;
  int loaded_something = 0;

  if (argc < 5) {
err_usage:
    fprintf(stderr, 
           "Usage: %s -d cc65_dbg_file -l cc65_lbl_file -t mame_trace_file [-f]\n"
           "\n"
           "-d cc65_dbg_file  : point to a cc65-generated .dbg file\n"
           "-l cc65_lbl_file  : point to a cc65-generated .lbl file\n"
           "-t mame_trace_file: point to a MAME debugger generated trace file\n"
           "-x start_addr     : specify start address, or 0x0 to start at top [default: 0x%x]\n"
           "-v                : verbose\n"
           "-p                : generate callgrind profile\n"
           "-f                : tail trace file\n"
           "\n"
           "Either -d or -l is required; -t is required.\n"
           "\n"
           "For profiles, generate tracefile with 'trace log.run,maincpu,noloop'\n",
           argv[0],DEFAULT_START_ADDR);
    exit(1);
  }
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-d") && i < argc - 1) {
      i++;
      load_syms(argv[i]);
      loaded_something = 1;
    } else if (!strcmp(argv[i], "-l") && i < argc - 1) {
      i++;
      load_lbls(argv[i]);
      loaded_something = 1;
    } else if (!strcmp(argv[i], "-t") && i < argc - 1) {
      i++;
      trace_file = argv[i];
    }  else if (!strcmp(argv[i], "-x") && i < argc - 1) {
      i++;
      start_addr = hex2int(argv[i]);
      if (start_addr == 0) {
        found_start_addr = 1;
      }
    } else if (!strcmp(argv[i], "-f")) {
      tail = 1;
    } else if (!strcmp(argv[i], "-v")) {
      verbose = 1;
    } else if (!strcmp(argv[i], "-p")) {
      do_callgrind = 1;
    } else {
      fprintf(stderr, "Unrecognized parameter %s\n", argv[i]);
      goto err_usage;
    }
  }

  if (!loaded_something) {
    goto err_usage;
  }

  /* Prepare everything */
  map_slocs_to_adresses();
  allocate_trace_counters();

  /* Do the thing */
  if (trace_file) {
    annotate_run(trace_file);
  } else {
    goto err_usage;
  }
  if (!found_start_addr) {
    fprintf(stderr, "Could not find entry point at start address 0x%04X.\n",
            start_addr);
  }
}
