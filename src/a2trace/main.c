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
#include "mame-params.h"

#define FIELD_WIDTH 40
#define DEFAULT_START_ADDR 0x803

int tail = 0;
int verbose = 0;
int do_callgrind = 0;
int found_start_addr = 0;
int cpu;
int cur_65816_bank = 0;
int dst_65816_bank = 0;

int start_addr = 0;
long inter_cycle = 0;

const char *count_sym_name = NULL;

unsigned char memory_contents[65536] = {0};

extern int read_from;
extern int write_to;
extern int lc_bank;

extern dbg_symbol ****gen_sym_cache;

static int hex2int(const char *hex) {
  if (tolower(hex[1]) != 'x') {
    char zerox[32];
    snprintf(zerox, 32, "0x%s", hex);
    return strtoul(zerox, (char**)0, 0);
  } else {
    return strtoul(hex, (char**)0, 0);
  }
}

static void tabulate(const char *buf, int len) {
  static char tbuf[80];
  int i = 0;
  if (do_callgrind)
    return;

  i = len + 1 - (buf ? strlen(buf) : 0);
  if (i > 78)
    i = 78;
  if (i < 0)
    i = 0;
  memset(tbuf, ' ', i);
  tbuf[i] = '\0';
  printf("%s", tbuf);
}

static int detect_tracelog(char *first_part, char *second_part) {
  char checkbuf[32];
  if (strlen(first_part) == 4 && hex2int(first_part) > 0) {
    fprintf(stderr, "Found addresses at index 0\n");
    return 0;
  }
  if(strlen(first_part) > 4 && strchr(first_part, ' ')) {
    char *try_addr = strchr(first_part, ' ') + 1;
    int val = hex2int(try_addr);

    snprintf(checkbuf, 5, "%04X", val);
    if (!strcmp(try_addr, checkbuf)) {
      int idx = (int)(try_addr - first_part);
      fprintf(stderr, "Found 6502 addresses at index %d\n", idx);
      cpu = CPU_6502;
      return idx;
    }
  }

  if(strchr(first_part, ' ')) {
    char *try_addr = strchr(first_part, ' ') + 1;
    int a_val = hex2int(try_addr);
    int b_val = hex2int(second_part);

    snprintf(checkbuf, 5, "%04X", b_val);
    if (a_val <= 0xFF && a_val >= 0x00 && !strcmp(second_part, checkbuf)) {
      int idx = (int)(try_addr - first_part);
      fprintf(stderr, "Found 65816 addresses at index %d\n", idx);
      cpu = CPU_65816;
      return idx;
    }
  }

  fprintf(stderr, "%s:%s\n", first_part, second_part);
  fprintf(stderr, "Can not parse log (address field not found)\n");
  exit(1);
}

static void update_regs(const char *buf, unsigned char *a, unsigned char *x, unsigned char *y, unsigned char *p) {
  *a = *x = *y = 0;
  if (strstr(buf, "A="))
    *a = (unsigned char)hex2int(strstr(buf, "A=") + 2);
  if (strstr(buf, "X="))
    *x = (unsigned char)hex2int(strstr(buf, "X=") + 2);
  if (strstr(buf, "Y="))
    *y = (unsigned char)hex2int(strstr(buf, "Y=") + 2);
  if (strstr(buf, "P="))
    *p = (unsigned char)hex2int(strstr(buf, "P=") + 2);
}

static int skipped_file(dbg_slocdef *sloc, char **exclude_list) {
  if (sloc != NULL) {
    char **excluded_pattern = exclude_list;
    const char *file = sloc_get_filename(sloc);
    while (excluded_pattern && *excluded_pattern) {
      if (strstr(file, *excluded_pattern)) {
        printf("file %s excluded\n", file);
        return 1;
      }
      excluded_pattern++;
    }
  }
  return 0;
}


static const char *print_flags(int flags) {
  static char *flagstr = NULL;

  if (flagstr == NULL)
    flagstr = strdup("........");

  flagstr[0] = ((flags & FLAG_N) != 0) ? 'N':'.';
  flagstr[1] = ((flags & FLAG_V) != 0) ? 'V':'.';
  flagstr[4] = ((flags & FLAG_D) != 0) ? 'D':'.';
  flagstr[5] = ((flags & FLAG_I) != 0) ? 'I':'.';
  flagstr[6] = ((flags & FLAG_Z) != 0) ? 'Z':'.';
  flagstr[7] = ((flags & FLAG_C) != 0) ? 'C':'.';

  return flagstr;
}

static void annotate_run(const char *file, char **skipped_files) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];
  char line_buf[BUF_SIZE];
  int cur_line = 0;
  int op_idx = -1;
  int print = 1;
  int skipped_lines = 0;

  if (fp == NULL) {
    fprintf(stderr, "Can not open file %s: %s\n", file, strerror(errno));
    exit(1);
  }

  if (!do_callgrind) {
    printf("Line #  ; Registers     ; Flags   ; Addr: Instruction            ;"
           " Resolved address                            "
           "; Instruction with symbol                    "
           "; C; Location\n");
  }
skip_to_start:
  line_buf[0] = '\0';

  while (fgets(buf, BUF_SIZE, fp) || tail) {
    char **parts = NULL;
    char *line, *tmp;
    int n_parts;
    int is_line;
    int buf_len;
    int a_mode;
    int cycles;

    buf_len = strlen(buf);

    if (buf_len == 0) {
      /* Wait for new data if tailing */
      if (tail) {
        clearerr(fp);
        usleep(5*1000);
        continue;
      } else {
        goto all_done;
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

    if ((tmp = strstr(line, "F8ROM:")) != NULL) {
      tmp[5] = '_'; /* Working around MAME here... */
    }
    n_parts = strsplit_in_place(line, ':', &parts);

    if (op_idx == -1 && n_parts >= 2) {
      op_idx = detect_tracelog(parts[0], parts[1]);
    }

    /* We want at least an "ADDR: op" */
    if ((cpu == CPU_6502 && n_parts >= 2) || (cpu == CPU_65816 && n_parts >= 3)) {
      int op_addr = 0;
      int param_addr = 0, orig_param_addr = 0;
      dbg_slocdef *sloc = NULL;
      dbg_symbol *instr_symbol = NULL;
      dbg_symbol *param_symbol = NULL;
      const char *instr, *arg;
      char comment[BUF_SIZE];
      char * cur_lineaddress;
      int addr_field;
      unsigned char a, x, y, p;
      int instruction_is_write = 0;
      cur_line++;

      /* get the op's address */
      if (cpu == CPU_6502) {
        addr_field = 0;
        cur_lineaddress = parts[0] + op_idx;
        op_addr = hex2int(cur_lineaddress);
      } else if (cpu == CPU_65816) {
        addr_field = 1;
        op_idx = strchr(parts[0], ' ') + 1 - parts[0];
        cur_65816_bank = hex2int(parts[0] + op_idx);
        cur_lineaddress = parts[1];
        op_addr = hex2int(cur_lineaddress);
      }
      /* If we've not yet seen the start address,
       * skip line
       */
      if (!found_start_addr) {
        if (start_addr == 0) {
          /* Autodetect */
          if (op_addr == 0x803 || op_addr == 0x2000 || op_addr == 0x4000) {
            start_addr = op_addr;
            found_start_addr = 1;
            cur_line = 1;
          }
        } else if (op_addr == start_addr) {
          found_start_addr = 1;
          cur_line = 1;
        }
        if (found_start_addr) {
          fprintf(stderr, "Found start address 0x%X.\n", start_addr);
          gen_sym_cache[start_addr][ROM][1] = generate_symbol("__MAIN_START__", start_addr, RAM, 1, NULL);
          start_tracing(cpu);
        }
        else
          goto skip_to_start;
      }

      /* get instruction */
      instr = parts[addr_field + 1] + 1; /* skip space */

      /* get arg if there's one */
      arg = strchr(instr, ' ');
      if (arg) {
        *(char *)arg = '\0';
        arg++;
        arg = fix_mame_param(arg);
      }

      if (op_addr >= 0) {
        /* Get our sloc only if the address is in RAM, out of LC or in LC page 2
         * Otherwise we're quite sure not to have a real symbol defined
         */
        if (cpu == CPU_65816) {
          /* https://github.com/TomHarte/CLK/wiki/Apple-IIgs-Memory-Map */
          read_from = cur_65816_bank >= 0xF0 ? ROM:RAM;
          if (read_from == RAM)
            sloc = sloc_get_for_addr(op_addr);
        }
        if (is_addr_in_cc65_user_bank(op_addr)) {
          sloc = sloc_get_for_addr(op_addr);
        }
        instr_symbol = symbol_get_by_addr(cpu, op_addr, read_from, lc_bank);
        if (!instr_symbol) {
          instr_symbol = generate_symbol(cur_lineaddress, op_addr, read_from, lc_bank, NULL);
        }
      }

      if (sloc && skipped_file(sloc, skipped_files)) {
        print = 0;
        skipped_lines++;
      } else {
        if (print == 0 && skipped_lines > 0) {
          printf("[... skipped %d lines ...]\n", skipped_lines);
        }
        print = 1;
        skipped_lines = 0;
      }

      /* Figure out A X Y and P */
      update_regs(parts[0], &a, &x, &y, &p);

      /* cache that, strcmps are expensive */
      instruction_is_write = is_instruction_write(instr);

      /* get addressing mode and cycles count.
       * Done here instead of in instructions.c to be
       * able to display a potentially problematic line
       */
      a_mode = instruction_get_addressing_mode(cpu, instr, arg);
      if ((cycles = get_cycles_for_instr(cpu, instr, a_mode, p)) < 0) {
        fprintf(stderr, "%s\n", buf);
        exit(1);
      }

      /* get param if it's numeric */
      if (cpu == CPU_65816 && arg && strlen(arg) == 6) {
        if (sscanf(arg, "%2X%4X", &dst_65816_bank, &param_addr) == 2) {
          goto addr_without_dollar;
        }
      }

      if (arg && arg[0] == '(' && arg[1] == '$') {
        int zpaddr = hex2int(arg + 2);
        int dest;
        param_addr = orig_param_addr = memory_contents[zpaddr] + (memory_contents[zpaddr+1] << 8);
        if (a_mode == ADDR_MODE_ZINDY) {
          param_addr = orig_param_addr + y;
        }

        if (cpu == CPU_6502 && param_addr >= 0) {
          if ((param_addr & 0xff00) != (orig_param_addr & 0xff00)
            && !instruction_is_write) {
            if (a_mode == ADDR_MODE_ZINDY) {
              /* cost for $nnnn, X ; $nnnn, Y with page crossing */
              cycles += 1;
            }
          }
        }
        dest = instruction_is_write ? write_to : read_from;
        param_symbol = symbol_get_by_addr(cpu, param_addr, dest, lc_bank);

      } else if (arg && arg[0] == '$') {
        if (strchr(arg, '\n'))
          *strchr(arg, '\n') = '\0';

        param_addr = 0;
        /* calculate offset */
        if (strstr(arg, ", x")) {
          param_addr = x;
        }
        if (strstr(arg, ", y")) {
          param_addr = y;
        }

        if (strchr(arg, ','))
          *strchr(arg, ',') = '\0';

        if (cpu == CPU_65816) {
          write_to = read_from;
        }

        if (cpu == CPU_6502 || strlen(arg + 1) <= 4) {
          orig_param_addr = hex2int(arg + 1); /* skip $ */
          param_addr += orig_param_addr;
        } else {
          sscanf(arg + 1, "%2X%4X", &dst_65816_bank, &param_addr);
          orig_param_addr = param_addr;
        }

        /* page cross penalty for read instruction */
        if (cpu == CPU_6502 && param_addr >= 0) {
          if ((param_addr & 0xff00) != (orig_param_addr & 0xff00)
            && !instruction_is_write) {
            if (a_mode == ADDR_MODE_ABSXY) {
              /* cost for $nnnn, X ; $nnnn, Y with page crossing */
              cycles += 1;
            }
          } else if ((op_addr & 0xff00) != (param_addr & 0xff00)
                   && is_instruction_condbranch(instr)) {
              /* cost for conditional branches with page crossing */
              cycles += 1;
          }
        }

addr_without_dollar:
        if (param_addr >= 0) {
          int dest;

          if (cpu == CPU_65816) {
            read_from = cur_65816_bank >= 0xF0 ? ROM:RAM;
            write_to = dst_65816_bank >= 0xF0 ? ROM:RAM;
          }

          dest = instruction_is_write ? write_to : read_from;
          param_symbol = symbol_get_by_addr(cpu, param_addr, dest, lc_bank);

          /* If we don't have a symbol, generate a dummy one */
          if (param_symbol == NULL) {
            int dest = instruction_is_write ? write_to : read_from;
            char new_str_addr[10];
            snprintf(new_str_addr, 10, "%04X", param_addr);
            param_symbol = generate_symbol(new_str_addr, param_addr, dest, lc_bank, n_parts > addr_field + 2 ? parts[addr_field + 2] : NULL);
            param_addr = symbol_get_addr(param_symbol);
          }
        } else {
          goto try_gen;
        }
      } else if (arg && arg[0] != '#') {
        int dest;
try_gen:
        /* Generate a dummy symbol. Its name will reference its address,
         * and where it will hit depending on current memory banking
         */
        dest = instruction_is_write ? write_to : read_from;
        param_symbol = generate_symbol(arg, param_addr, dest, lc_bank, n_parts > addr_field + 2 ? parts[addr_field + 2] : NULL);
        param_addr = symbol_get_addr(param_symbol);
      }

      /* Update memory content if needed */
      if (instruction_is_write
       && a_mode != ADDR_MODE_ACC && a_mode != ADDR_MODE_IMMEDIATE) {
        if (param_addr > 65535) {
          fprintf(stderr, "ignoring %s write to address 0x%04x mode %d at line %d\n", instr, param_addr, a_mode, cur_line);
        } else if (!strcmp(instr, "dec")) {
          memory_contents[param_addr]--;
        } else if (!strcmp(instr, "inc")) {
          memory_contents[param_addr]++;
        } else if (!strcmp(instr, "asl")) {
          memory_contents[param_addr] <<= 1;
        } else if (!strcmp(instr, "lsr")) {
          memory_contents[param_addr] >>= 1;
        } else if (!strcmp(instr, "rol")) {
          memory_contents[param_addr] <<= 1;
        } else if (!strcmp(instr, "ror")) {
          memory_contents[param_addr] >>= 1;
        } else if (!strcmp(instr, "sta")) {
          memory_contents[param_addr] = a;
        } else if (!strcmp(instr, "stx")) {
          memory_contents[param_addr] = x;
        } else if (!strcmp(instr, "sty")) {
          memory_contents[param_addr] = y;
        } else if (!strcmp(instr, "stz")) {
          memory_contents[param_addr] = 0;
        } else if (!strcmp(instr, "trb")) {
          memory_contents[param_addr] = ~a & memory_contents[param_addr];
        } else if (!strcmp(instr, "tsb")) {
          memory_contents[param_addr] = a | memory_contents[param_addr];
        }
      }

      int backtab = 0;
      /* Print the line as-is */
      if (print && !do_callgrind) {
        printf("%08d; A=%02X X=%02X Y=%02X; %s; %s", cur_line, a, x, y, print_flags(p), line_buf + op_idx);
        if (strlen(line_buf) > op_idx + 28) {
          backtab = op_idx + 29 - strlen(line_buf);
        }
      }

      /* Profile if needed */
      if (do_callgrind && update_call_counters(cpu, op_addr, instr, param_addr, cycles, cur_line) < 0) {
        fprintf(stderr, "; Error popping call tree at trace line %d\n", cur_line);
      }

      /* Analyse instruction to follow memory banking */
      if (analyze_instruction(cpu, op_addr, instr, param_addr, comment)
       || (!sloc && !instr_symbol && !param_symbol)) {
        /* print either banking comment or finish the line if we have zero data about it */
        if (print && !do_callgrind) {
          tabulate(line_buf, op_idx + 28);
          printf("; *%s*\n", comment);
        }
      } else {
        if (print && !do_callgrind) {
          tabulate(line_buf, op_idx + 28);
          printf("; adr: ");
        }
        /* Display the instruction's symbol */
        if (instr_symbol) {
          if (print && !do_callgrind) {
            char count_buf[32];
            int count_buf_len = 0;
            const char *sym_name = symbol_get_name(instr_symbol);
            printf("%s", sym_name);

            /* Show cycle count between two PC matching symbol name */
            if (count_sym_name != NULL && strstr(sym_name, " +") == NULL &&
                !strncmp(sym_name, count_sym_name, strlen(count_sym_name))) {
              snprintf(count_buf, 31, " (%lu cyc)", inter_cycle);
              count_buf_len = strlen(count_buf);
              printf("%s", count_buf);
              inter_cycle = 0;
            }
            tabulate(sym_name, FIELD_WIDTH + backtab - count_buf_len);
          }
        } else if (print) {
          tabulate(NULL, FIELD_WIDTH + backtab);
        }

        /* Print the argument's symbol */
        if (print && !do_callgrind)
          printf("%s ", instr);
        if (param_symbol) {
          if (print && !do_callgrind) {
            printf("%s", symbol_get_name(param_symbol));
            tabulate(symbol_get_name(param_symbol), FIELD_WIDTH);
          }
        } else if (arg) {
          if (print && !do_callgrind) {
            printf("%s", arg);
            tabulate(arg, FIELD_WIDTH);
          }
        } else if (print) {
          tabulate(NULL, FIELD_WIDTH);
        }

        if (print && !do_callgrind) {
          printf("%d ", cycles);
          inter_cycle += cycles;
        }
        /* Print the source location */
        if (print && sloc && !do_callgrind) {
          printf("(%s:%d)", sloc_get_filename(sloc), sloc_get_line(sloc));
        } else if (print && !do_callgrind) {
          printf(" ?");
        }
        if (print && !do_callgrind)
          printf("\n");
      }
    } else {
      /* Very probably a "(Loop for n instructions)" message from MAME */
      if (!do_callgrind)
        printf("%s\n", line_buf);
      if (do_callgrind && !strstr(line_buf, "interrupted at") && line_buf[0] != '\0') {
        fprintf(stderr, "Error: unexpected line at line %d :\n'%s'\n", cur_line, line_buf);
        fprintf(stderr, "Profiles must be generated from MAME with full traces:\n"
               "trace log.run,maincpu,noloop'\n");
      }
    }

    free(line);
    free(parts);

    /* reset read buffers */
    line_buf[0] = '\0';
    buf[0] = '\0';
  }

all_done:
  fclose(fp);

  /* Finish the counts */
  if (do_callgrind)
    finalize_call_counters();
}

static char **parse_list(char *list) {
  char **files = NULL;
  int n_files = strsplit_in_place(list, ',', &files);
  files = realloc(files, (n_files + 1) * sizeof(char *));
  files[n_files] = NULL;

  return files;
}

int main(int argc, char *argv[]) {
  int i;
  char *trace_file = NULL;
  char **excluded_files = NULL;
  char **excluded_segments = NULL;
  char **skip_files = NULL;
  int loaded_something = 0;

  if (argc < 5) {
err_usage:
    fprintf(stderr,
           "This tool is quite limited to interpreting CPU trace logs under certain\n"
           "conditions:\n"
           " - It reads trace files generated by MAME, expecting a certain format\n"
           " - For programs compiled using cc65 (`ld65 -g -Wl --dbgFile,program.dbg`)\n"
           " - For programs running on Apple2.\n"
           "\n"
           "Usage: %s -d cc65_dbg_file -l cc65_lbl_file -t mame_trace_file [-f]\n"
           "\n"
           "--exclude-files lst: exclude symbols from files matching lst (lst: pattern1,pattern2,...,patternN);\n"
           "                     Use before -d\n"
           "--exclude-segs  lst: exclude symbols from segments in lst (lst: CODE,RODATA,...);\n"
           "                     Use before -d\n"
           "--skip-over     lst: skip tracking inside files matching lst (lst: pattern1,pattern2,...,patternN);\n"
           "                     Use before -d\n"
           "-d  file.dbg       : point to a cc65-generated .dbg file (best)\n"
           "-l  file.lbl       : or point to a cc65-generated .lbl file (inferior)\n"
           "-t  file.tr        : point to a MAME debugger generated trace file\n"
           "-x  start_addr     : specify start address, [default: auto-detect]\n"
           "-nx                : start at beginning without looking for a start address\n"
           "-c SYMBOL_SUBSTR   : count cycles between two PC addresses, by symbol name start\n"
           "-v                 : verbose\n"
           "-p                 : generate callgrind profile\n"
           "-f                 : tail trace file\n"
           "\n"
           "Either -d or -l is required; -t is required.\n"
           "\n"
           "Generate trace files in MAME debugger using:\n"
           "  trace program.tr,maincpu,noloop,{tracelog \"A=%%02X,X=%%02X,Y=%%02X,P=%%02X \",a,x,y,p}\n\n",
           argv[0]);
    exit(1);
  }
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-d") && i < argc - 1) {
      i++;
      load_syms(argv[i], excluded_files, excluded_segments);
      loaded_something = 1;
    } else if (!strcmp(argv[i], "-l") && i < argc - 1) {
      i++;
      load_lbls(argv[i]);
      loaded_something = 1;
    } else if (!strcmp(argv[i], "-t") && i < argc - 1) {
      i++;
      trace_file = argv[i];
    } else if (!strcmp(argv[i], "--exclude-files") && i < argc - 1) {
      i++;
      excluded_files = parse_list(argv[i]);
    } else if (!strcmp(argv[i], "--exclude-segs") && i < argc - 1) {
      i++;
      excluded_segments = parse_list(argv[i]);
    } else if (!strcmp(argv[i], "--skip-over") && i < argc - 1) {
      i++;
      skip_files = parse_list(argv[i]);
    }  else if (!strcmp(argv[i], "-x") && i < argc - 1) {
      i++;
      start_addr = hex2int(argv[i]);
    }  else if (!strcmp(argv[i], "-c") && i < argc - 1) {
      i++;
      count_sym_name = argv[i];
    } else if (!strcmp(argv[i], "-nx")) {
      found_start_addr = 1;
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
  if (do_callgrind && found_start_addr) {
    fprintf(stderr, "-p and -nx flags are incompatible:\n"
            "Profiling has to start from start address.\n");
    exit(1);
  }

  if (!loaded_something) {
    goto err_usage;
  }

  /* Prepare everything */
  map_slocs_to_adresses(excluded_segments);
  allocate_trace_counters();

  if (found_start_addr) {
    start_tracing(cpu);
  }
  /* Do the thing */
  if (trace_file) {
    annotate_run(trace_file, skip_files);
  } else {
    goto err_usage;
  }
  if (!found_start_addr) {
    fprintf(stderr, "Could not find entry point at start address ");
    if (start_addr != 0)
      fprintf(stderr, "0x%04X.\n", start_addr);
    else
      fprintf(stderr, "(auto-detection failed).\n");
  }
}
