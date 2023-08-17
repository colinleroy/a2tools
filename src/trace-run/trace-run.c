#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "strsplit.h"
#include "slist.h"

#define BUF_SIZE 4096
#define STORAGE_SIZE 200000

#define FIELD_WIDTH 40
typedef struct _line dbg_line;
typedef struct _span dbg_span;
typedef struct _file dbg_file;
typedef struct _segment dbg_segment;
typedef struct _symbol dbg_symbol;
typedef struct _address dbg_address;

int tail = 0;

#define RAM 0
#define ROM 1
#define NONE 2

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
  "stz", "trb", "tsb"
  NULL
};

int read_from = ROM;
int write_to  = NONE;
int lc_bank   = 1;

struct _line {
  int file;
  int line_number;
  char *spans;
};

struct _span {
  int segment;
  int start;
  int size;
  int type;
};

struct _file {
  char *name;
  int size;
};

struct _segment {
  char *name;
  int start;
  int size;
};

struct _symbol {
  char *name;
  int size;
  int segment;
};

struct _address {
  dbg_file *file;
  dbg_line *line;
};

static dbg_line **lines = NULL;
static dbg_span **spans = NULL;
static dbg_file **files = NULL;
static dbg_segment **segs = NULL;
static dbg_symbol **symbols = NULL;
static dbg_address **addresses = NULL;

static int get_int_val(const char *str) {
  char *val = strchr(str, '=');
  if (val) {
    return atol(val + 1);
  }
  return -1;
}

static int get_hex_val(const char *str) {
  char *val = strchr(str, '=');
  if (val) {
    return strtoul(val + 1, (char**)0, 0);
  }
  return -1;
}

static char *get_str_val(const char *str) {
  char *val = strchr(str, '=');
  if (val) {
    return (val + 1);
  }
  return NULL;
}

static void insert_line(char **parts, int n_parts) {
  int id = -1;
  int file = -1;
  int line_number = -1;
  char *span = NULL;
  int i;

  for (i = 0; i < n_parts; i++) {
    if (!strncmp(parts[i], "id=", 3))
      id = get_int_val(parts[i]);
    if (!strncmp(parts[i], "file=", 5))
      file = get_int_val(parts[i]);
    if (!strncmp(parts[i], "line=", 5))
      line_number = get_int_val(parts[i]);
    if (!strncmp(parts[i], "span=", 5)) {
      span = get_str_val(parts[i]);
      if (span && strchr(span, '\n'))
        *strchr(span, '\n') = '\0';
    }
  }
  if (id != -1 && lines[id] == NULL && span != NULL) {
    lines[id] = malloc(sizeof(dbg_line));
    lines[id]->file = file;
    lines[id]->line_number = line_number;
    lines[id]->spans = strdup(span);
    //printf("inserted line %d (spans %s)\n", id, span);
  }
}

static void insert_span(char **parts, int n_parts) {
  int id = -1;
  int segment = -1;
  int start = -1;
  int size = -1;
  int i;

  for (i = 0; i < n_parts; i++) {
    if (!strncmp(parts[i], "id=", 3))
      id = get_int_val(parts[i]);
    if (!strncmp(parts[i], "seg=", 4))
      segment = get_int_val(parts[i]);
    if (!strncmp(parts[i], "start=", 6))
      start = get_int_val(parts[i]);
    if (!strncmp(parts[i], "size=", 6))
      size = get_int_val(parts[i]);
  }
  if (id != -1 && spans[id] == NULL && spans != NULL) {
    spans[id] = malloc(sizeof(dbg_span));
    spans[id]->segment = segment;
    spans[id]->start = start;
    spans[id]->size = size;
    //printf("inserted span %d (start %d)\n", id, start);
  }
}

static void insert_file(char **parts, int n_parts) {
  int id = -1;
  char *name = NULL;
  int size = -1;
  int i;

  for (i = 0; i < n_parts; i++) {
    if (!strncmp(parts[i], "id=", 3))
      id = get_int_val(parts[i]);
    if (!strncmp(parts[i], "size=", 5))
      size = get_int_val(parts[i]);
    if (!strncmp(parts[i], "name=", 5)) {
      name = get_str_val(parts[i]);
      if (name && strchr(name, '\n'))
        *strchr(name, '\n') = '\0';
    }
  }
  if (id != -1 && files[id] == NULL && name != NULL) {
    files[id] = malloc(sizeof(dbg_file));
    files[id]->name = strdup(name);
    files[id]->size = size;
    //printf("inserted file %d (name %s)\n", id, name);
  }
}

static void insert_segment(char **parts, int n_parts) {
  int id = -1;
  char *name = NULL;
  int start = -1;
  int size = -1;
  int i;

  for (i = 0; i < n_parts; i++) {
    if (!strncmp(parts[i], "id=", 3))
      id = get_int_val(parts[i]);
    if (!strncmp(parts[i], "start=", 5))
      start = get_hex_val(parts[i]);
    if (!strncmp(parts[i], "size=", 5))
      size = get_hex_val(parts[i]);
    if (!strncmp(parts[i], "name=", 5)) {
      name = get_str_val(parts[i]);
      if (name && strchr(name, '\n'))
        *strchr(name, '\n') = '\0';
    }
  }
  if (id != -1 && segs[id] == NULL && name != NULL) {
    segs[id] = malloc(sizeof(dbg_segment));
    segs[id]->name = strdup(name);
    segs[id]->start = start;
    segs[id]->size = size;
    //printf("inserted segment %d (name %s, start %d)\n", id, name, start);
  }
}

static void insert_symbol(char **parts, int n_parts) {
  int id = -1;
  char *name = NULL;
  int size = -1;
  int i;

  for (i = 0; i < n_parts; i++) {
    if (!strncmp(parts[i], "val=", 3))
      id = get_hex_val(parts[i]);
    if (!strncmp(parts[i], "size=", 5))
      size = get_hex_val(parts[i]);
    if (!strncmp(parts[i], "name=", 5)) {
      name = get_str_val(parts[i]);
      if (name && strchr(name, '\n'))
        *strchr(name, '\n') = '\0';
    }
  }
  if (id != -1 && symbols[id] == NULL && name != NULL) {
    for (i = 0; i < size; i++) {
      symbols[id + i] = malloc(sizeof(dbg_symbol));
      if (i == 0) {
        symbols[id + i]->name = strdup(name);
      } else {
        symbols[id + i]->name = malloc(128);
        snprintf(symbols[id + i]->name, 128, "%s+%d", name, i);
      }
      symbols[id + i]->size = size;
      //printf("inserted symbol %d (name %s, size %d)\n", id + i, name, size);
    }
  }
}

static void load_syms(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];

  lines      = malloc(sizeof(dbg_line *) * STORAGE_SIZE);
  spans      = malloc(sizeof(dbg_span *) * STORAGE_SIZE);
  files      = malloc(sizeof(dbg_file *) * STORAGE_SIZE);
  segs       = malloc(sizeof(dbg_segment *) * STORAGE_SIZE);
  symbols    = malloc(sizeof(dbg_symbol *) * STORAGE_SIZE);
  addresses  = malloc(sizeof(dbg_address *) * STORAGE_SIZE);

  memset(lines,     0, sizeof(dbg_line *) * STORAGE_SIZE);
  memset(spans,     0, sizeof(dbg_span *) * STORAGE_SIZE);
  memset(files,     0, sizeof(dbg_file *) * STORAGE_SIZE);
  memset(segs,      0, sizeof(dbg_segment *) * STORAGE_SIZE);
  memset(symbols,   0, sizeof(dbg_symbol *) * STORAGE_SIZE);
  memset(addresses, 0, sizeof(dbg_address *) * STORAGE_SIZE);

  while (fgets(buf, BUF_SIZE, fp)) {
    char **parts;
    int n_parts;

    n_parts = strsplit_in_place(buf, '\t', &parts);
    if (n_parts == 2) {
      char **subparts;
      int n_subparts;

      n_subparts = strsplit_in_place(parts[1], ',', &subparts);

      if (!strcmp(parts[0], "line")) {
        insert_line(subparts, n_subparts);
      }
      if (!strcmp(parts[0], "span")) {
        insert_span(subparts, n_subparts);
      }
      if (!strcmp(parts[0], "file")) {
        insert_file(subparts, n_subparts);
      }
      if (!strcmp(parts[0], "seg")) {
        insert_segment(subparts, n_subparts);
      }
      if (!strcmp(parts[0], "sym")) {
        insert_symbol(subparts, n_subparts);
      }
      free(subparts);
    }
    free(parts);
  }
  fclose(fp);
}

static void load_lbls(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];

  while (fgets(buf, BUF_SIZE, fp)) {
    char **parts;
    int n_parts;

    n_parts = strsplit_in_place(buf, ' ', &parts);
    if (n_parts == 3) {
      char zerox[32];
      char *name = parts[2];
      int addr;
      /* get address */
      snprintf(zerox, 32, "0x%s", parts[1]);
      addr = strtoul(zerox, (char**)0, 0);
      if (addr > 0) {
        if (symbols[addr] == NULL) {
          if (strchr(name, '\n'))
            *strchr(name, '\n') = '\0';

          symbols[addr] = malloc(sizeof(dbg_symbol));
          symbols[addr]->name = strdup(name);
          symbols[addr]->size = 0;
          //printf("Added %s at %04X\n", name, addr);
        }
      }
    }
    free(parts);
  }
  fclose(fp);
}

static void cleanup_symbols(void) {
  int i;
  for (i = 0; i < STORAGE_SIZE; i++) {
    dbg_symbol *symbol = symbols[i];
    char *tmp, *quote;

    if (!symbol || symbol->name[0] != '"')
      continue;

    /* strip first quote */
    tmp = strdup(symbol->name + 1);
    /* strip the other ones */
    while ((quote = strchr(tmp, '"')) != NULL)
      *quote = ' ';

    free(symbol->name);
    symbol->name = tmp;
  }
}

static void map_lines_to_adresses(void) {
  int l;
  for (l = 0; l < STORAGE_SIZE; l++) {
    dbg_line *line = lines[l];
    dbg_file *file;
    char **c_spans;
    int i, n_spans;

    if (line == NULL) {
      continue;
    }

    file = files[line->file];
    n_spans = strsplit(line->spans, '+', &c_spans);
    for (i = 0; i < n_spans; i++) {
      int span_num     = atoi(c_spans[i]);
      dbg_span *span;
      dbg_segment *seg;
      int addr;

      span = spans[span_num];
      if (!span) {
        continue;
      }

      seg = segs[span->segment];
      if (!seg) {
        continue;
      }

      addr = seg->start + span->start;

      if (addresses[addr]) {
        //printf("Warning: Address 0x%04x already set\n", addr);
      } else {
        addresses[addr] = malloc(sizeof (dbg_address));
        addresses[addr]->file = file;
        addresses[addr]->line = line;
        //printf("Mapped 0x%04X => %s:%d\n", addr, file->name, line->line_number);
      }
      free(c_spans[i]);
    }
    free(c_spans);
  }
}

static int analyze_op(int op_addr, char *op, int param_addr) {
  static int bit_count = 0;
  static int last_bit = 0;
  int parsed = 0, bank_switch = 0;

  if (!strcmp(op, "bit")) {
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
    printf(" ; BANK SWITCH: read from %s, write to %s, LC bank %d",
            mem_str[read_from], mem_str[write_to], lc_bank);
  }
  return parsed;
}

int is_op_write(char *op) {
  int i;
  for (i = 0; write_instructions[i] != NULL; i++) {
    if (!strcmp(write_instructions[i], op)) {
      return 1;
    }
  }
  return 0;
}

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
      dbg_address *address = NULL;
      dbg_symbol *op_symbol = NULL;
      dbg_symbol *param_symbol = NULL;
      char *op, *arg;

      char zerox[32];

      /* get address */
      snprintf(zerox, 32, "0x%s", parts[0]);
      op_addr = strtoul(zerox, (char**)0, 0);

      /* get op */
      op = parts[1] + 1; /* skip space */
      arg = strchr(op, ' ');
      if (arg) {
        *arg = '\0';
        arg++;
      }

      if (op_addr >= 0) {
        /* Get our symbol only if the address is in RAM, out of LC or in LC page 2 */
        if ((read_from == RAM && (op_addr < 0xD000 || op_addr >= 0xDFFF))
         || (read_from == RAM && op_addr >= 0xD000 && op_addr < 0xDFFF && lc_bank == 2)) {
          address = addresses[op_addr];
          op_symbol = symbols[op_addr];
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
          if (param_addr >= 0xD000 && lc_bank == 2) {
            int op_writes = is_op_write(op);
            if (write_to == RAM && op_writes) {
              /* We're writing to LC bank2 */
              param_symbol = symbols[param_addr];
            } else if (read_from == RAM && !op_writes) {
              param_symbol = symbols[param_addr];
            }
          } else if (param_addr < 0xD000) {
            param_symbol = symbols[param_addr];
          }
        }
      }

      printf("%s", line_buf);

      if (analyze_op(op_addr, op, param_addr) || (!address && !op_symbol && !param_symbol)) {
        printf("\n");
      } else {
        int i;
        for (i = strlen(line_buf); i < 18; i++) printf(" ");
        printf("; OP: ");

        if (op_symbol) {
          printf("%s", op_symbol->name);
          for (i = strlen(op_symbol->name); i < FIELD_WIDTH; i++) printf(" ");
        } else {
          for (i = 0; i < FIELD_WIDTH; i++) printf(" ");
        }

        printf("ARG: ");
        if (param_symbol) {
          printf("%s", param_symbol->name);
          for (i = strlen(param_symbol->name); i < FIELD_WIDTH; i++) printf(" ");
        } else if (arg && arg[0] == '$') {
          printf("%s", zerox);
          for (i = strlen(zerox); i < FIELD_WIDTH; i++) printf(" ");
        } else {
          for (i = 0; i < FIELD_WIDTH; i++) printf(" ");
        }

        if (address) {
          i = printf("OP loc: %s:%d", address->file->name, address->line->line_number);
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
  cleanup_symbols();
  map_lines_to_adresses();
  if (trace_file) {
    annotate_run(trace_file);
  } else {
    goto err_usage;
  }
}
