#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "strsplit.h"
#include "slist.h"
#include "general.h"
#include "symbols.h"
#include "instructions.h"

int tail = 0;


#define FIELD_WIDTH 40

static void annotate_run(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];
  char line_buf[BUF_SIZE];

  line_buf[0] = '\0';

  while (fgets(buf, BUF_SIZE, fp) || tail) {
    char **parts = NULL;
    char *line;
    int n_parts;
    int is_line;
    int buf_len;

    buf_len = strlen(buf);
    if (buf_len == 0) {
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

    if (n_parts >= 2) {
      int op_addr = 0;
      int param_addr = 0;
      dbg_slocdef *sloc = NULL;
      dbg_symbol *instr_symbol = NULL;
      dbg_symbol *param_symbol = NULL;
      char *instr, *arg;
      char comment[BUF_SIZE];

      char zerox[32];

      /* get SLOC */
      snprintf(zerox, 32, "0x%s", parts[0]);
      op_addr = strtoul(zerox, (char**)0, 0);

      /* get instr */
      instr = parts[1] + 1; /* skip space */
      arg = strchr(instr, ' ');
      if (arg) {
        *arg = '\0';
        arg++;
      }

      if (op_addr >= 0) {
        /* Get our symbol only if the address is in RAM, out of LC or in LC page 2 */
        if (is_addr_in_cc65_user_bank(op_addr)) {
          sloc = sloc_get_for_addr(op_addr);
          instr_symbol = symbol_get_by_addr(op_addr);
        }
      }

      /* get param */
      if (arg && arg[0] == '$') {
        if (strchr(arg, '\n'))
          *strchr(arg, '\n') = '\0';
        if (strchr(arg, ','))
          *strchr(arg, ',') = '\0';
        snprintf(zerox, 32, "0x%s", arg + 1); /* skip $ */

        param_addr = strtoul(zerox, (char**)0, 0);
        if (param_addr >= 0) {
          if (param_addr >= 0xD000 && is_lc_user_page_on()) {
            if (get_ram_write_destination() == RAM && is_instruction_write(instr)) {
              /* We're writing to LC bank2 */
              param_symbol = symbol_get_by_addr(param_addr);
            } else if (get_ram_read_source() == RAM && !is_instruction_write(instr)) {
              param_symbol = symbol_get_by_addr(param_addr);
            }
          } else if (param_addr < 0xD000) {
            param_symbol = symbol_get_by_addr(param_addr);
          }
        }
      }

      printf("%s", line_buf);

      update_call_counters(instr, param_addr);

      if (analyze_instruction(op_addr, instr, param_addr, comment) || (!sloc && !instr_symbol && !param_symbol)) {
        printf("%s\n", comment);
      } else {
        int i;
        for (i = strlen(line_buf); i < 18; i++) printf(" ");
        printf("; OP: ");

        if (instr_symbol) {
          printf("%s", symbol_get_name(instr_symbol));
          for (i = strlen(symbol_get_name(instr_symbol)); i < FIELD_WIDTH; i++) printf(" ");
        } else {
          for (i = 0; i < FIELD_WIDTH; i++) printf(" ");
        }

        printf("ARG: ");
        if (param_symbol) {
          printf("%s", symbol_get_name(param_symbol));
          for (i = strlen(symbol_get_name(param_symbol)); i < FIELD_WIDTH; i++) printf(" ");
        } else if (arg && arg[0] == '$') {
          printf("%s", zerox);
          for (i = strlen(zerox); i < FIELD_WIDTH; i++) printf(" ");
        } else {
          for (i = 0; i < FIELD_WIDTH; i++) printf(" ");
        }

        if (sloc) {
          i = printf("OP loc: %s:%d", sloc_get_filename(sloc), sloc_get_line(sloc));
        }
        printf("\n");
      }
    } else {
      printf("%s\n", line_buf);
    }

    free(line);
    free(parts);

    /* reset read buffers */
    line_buf[0] = '\0';
    buf[0] = '\0';
  }

  fclose(fp);

  print_counters();
}

int main(int argc, char *argv[]) {
  int i;
  char *trace_file = NULL;

  if (argc < 7) {
err_usage:
    printf("Usage: %s -d cc65_dbg_file -l cc65_lbl_file -t mame_trace_file [-f]\n", argv[0]);
    printf("\n"
           "-d cc65_dbg_file:   point to a cc65-generated .dbg file\n"
           "-l cc65_lbl_file:   point to a cc65-generated .lbl file\n"
           "-t mame_trace_file: point to a MAME debugger generated trace file\n"
           "-f                : tail trace file\n");
    exit(1);
  }
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-d") && i < argc - 1) {
      i++;
      load_syms(argv[i]);
    }
    if (!strcmp(argv[i], "-l") && i < argc - 1) {
      i++;
      load_lbls(argv[i]);
    }
    if (!strcmp(argv[i], "-t") && i < argc - 1) {
      i++;
      trace_file = argv[i];
    }
    if (!strcmp(argv[i], "-f")) {
      tail = 1;
    }
  }

  map_slocs_to_adresses();
  allocate_trace_counters();

  if (trace_file) {
    annotate_run(trace_file);
  } else {
    goto err_usage;
  }
}
