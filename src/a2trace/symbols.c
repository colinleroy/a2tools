#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "strsplit.h"
#include "slist.h"
#include "general.h"
#include "instructions.h"
#include "symbols.h"

/* Internal structures to parse cc65 .dbg files */
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
  int addr;
  int real_addr;
  char *name;
  int size;
  int segment;
  int mem;
  int lcbank;
};

struct _slocdef {
  dbg_file *file;
  dbg_line *line;
};

extern int start_addr;

/* Storage */
static dbg_line **lines = NULL;
static dbg_span **spans = NULL;
static dbg_file **files = NULL;
static dbg_segment **segs = NULL;
static dbg_symbol **symbols = NULL;
static dbg_slocdef **slocs = NULL;

static dbg_symbol **gen_symbols = NULL;
static int num_gen_symbols = 0;

/* Cache for generated symbols */
dbg_symbol ****gen_sym_cache = NULL;

/* Conversion helpers */
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

/* Add a line (as in line number in a source file) to data */
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

/* Add a span - I have no idea what it represents */
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

/* Add a source file */
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

/* Add a (code) segment */
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

/* Add a symbol */
static void insert_symbol(char **parts, int n_parts) {
  int id = -1;
  char *name = NULL;
  int size = -1;
  int i;

  /* Parse fields */
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

  if (size <= 0)
    size = 1;

  /* Insert it, and when we have a size, insert it
   * with all offsets */
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
      symbols[id + i]->lcbank = (id + i < 0xD000 || id + i > 0xDFFF) ? 1 : 2;
      symbols[id + i]->mem = RAM;
      symbols[id + i]->addr = id + i;
      symbols[id + i]->real_addr = id + i;
      //printf("inserted symbol %d (name %s, size %d)\n", id + i, name, size);
    }
  }
}

/* Remove quotes and trailing spaces from
 * symbols and source files */
static void cleanup_data(void) {
  int i;
  for (i = 0; i < STORAGE_SIZE; i++) {
    dbg_symbol *symbol = symbols[i];
    dbg_file *file = files[i];

    char *tmp, *quote;

    if (!symbol || symbol->name[0] != '"')
      goto cleanup_file;

    /* strip first quote */
    tmp = strdup(symbol->name + 1);
    /* strip the other ones */
    while ((quote = strchr(tmp, '"')) != NULL)
      *quote = ' ';

    free(symbol->name);
    symbol->name = tmp;
    if (strchr(symbol->name, ' ')) {
      char *char_after = strchr(symbol->name, ' ');
      char_after++;
      if (*char_after == '\0')
        *strchr(symbol->name, ' ') = '\0';
    }

cleanup_file:
    if (!file || file->name[0] != '"')
      continue;

    /* strip first quote */
    tmp = strdup(file->name + 1);
    /* strip the other ones */
    while ((quote = strchr(tmp, '"')) != NULL)
      *quote = ' ';

    free(file->name);
    file->name = tmp;

    if (strchr(file->name, ' ')) {
      char *char_after = strchr(file->name, ' ');
      char_after++;
      if (*char_after == '\0')
        *strchr(file->name, ' ') = '\0';
    }
  }
}

static void allocate_data(void) {
  if (lines != NULL) {
    /* Already done */
    return;
  }

  /* Allocate sufficient data. This is not state of the
   * art. */
  lines      = malloc(sizeof(dbg_line *) * STORAGE_SIZE);
  spans      = malloc(sizeof(dbg_span *) * STORAGE_SIZE);
  files      = malloc(sizeof(dbg_file *) * STORAGE_SIZE);
  segs       = malloc(sizeof(dbg_segment *) * STORAGE_SIZE);
  symbols    = malloc(sizeof(dbg_symbol *) * STORAGE_SIZE);
  gen_symbols= malloc(sizeof(dbg_symbol *) * STORAGE_SIZE);
  slocs      = malloc(sizeof(dbg_slocdef *) * STORAGE_SIZE);
  
  /* Zero all */
  memset(lines,       0, sizeof(dbg_line *) * STORAGE_SIZE);
  memset(spans,       0, sizeof(dbg_span *) * STORAGE_SIZE);
  memset(files,       0, sizeof(dbg_file *) * STORAGE_SIZE);
  memset(segs,        0, sizeof(dbg_segment *) * STORAGE_SIZE);
  memset(symbols,     0, sizeof(dbg_symbol *) * STORAGE_SIZE);
  memset(gen_symbols, 0, sizeof(dbg_symbol *) * STORAGE_SIZE);
  memset(slocs,       0, sizeof(dbg_slocdef *) * STORAGE_SIZE);

  /* Prepare generate symbols cache */
  gen_sym_cache = malloc(sizeof(dbg_symbol *) * STORAGE_SIZE);
  for (int i = 0; i < STORAGE_SIZE; i++) {
    /* mem (RAM/ROM/NONE) */
    gen_sym_cache[i] = malloc(sizeof(dbg_symbol *) * 3);
    /* LC 1/2 */
    gen_sym_cache[i][0] = malloc(sizeof(dbg_symbol *) * 3);
    gen_sym_cache[i][1] = malloc(sizeof(dbg_symbol *) * 3);
    gen_sym_cache[i][2] = malloc(sizeof(dbg_symbol *) * 3);
    memset(gen_sym_cache[i][0], 0, sizeof(dbg_symbol *) * 3);
    memset(gen_sym_cache[i][1], 0, sizeof(dbg_symbol *) * 3);
    memset(gen_sym_cache[i][2], 0, sizeof(dbg_symbol *) * 3);
  }
}

void load_syms(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];

  if (fp == NULL) {
    fprintf(stderr, "Can not open file %s: %s\n", file, strerror(errno));
    exit(1);
  }

  allocate_data();

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
      if (!strcmp(parts[0], "exp")) {
        insert_symbol(subparts, n_subparts);
      }
      free(subparts);
    }
    free(parts);
  }
  fclose(fp);

  /* add IRQ handlers, both in RAM and ROM syms */
  gen_sym_cache[ROM_IRQ_ADDR[CPU_6502]][ROM][1] = generate_symbol("__ROM_IRQ", ROM_IRQ_ADDR[CPU_6502], RAM, 1, "0xC803");
  gen_sym_cache[ROM_IRQ_ADDR[CPU_65816]][ROM][1] = generate_symbol("__ROM_IRQ", ROM_IRQ_ADDR[CPU_65816], RAM, 1, "0xC366");
  gen_sym_cache[PRODOS_IRQ_ADDR][ROM][1] = generate_symbol("__ProDOS_IRQ", PRODOS_IRQ_ADDR, RAM, 1, "0xBFEB");
  cleanup_data();
}

/* Add labels from lbl file. Needed because I couldn't
 * find a way to map static variables to their addresses
 * in the .dbg file */
void load_lbls(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];

  if (fp == NULL) {
    fprintf(stderr, "Can not open file %s: %s\n", file, strerror(errno));
    exit(1);
  }

  allocate_data();

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
          symbols[addr]->addr = addr;
          symbols[addr]->real_addr = addr;
          symbols[addr]->size = 0;
          symbols[addr]->mem = RAM;
          symbols[addr]->lcbank = (addr < 0xD000 || addr > 0xDFFF) ? 1 : 2;
          
          //printf("Added %s at %04X\n", name, addr);
        }
      }
    }
    free(parts);
  }
  fclose(fp);
  cleanup_data();
}

/* Map symbols and their source locations */
void map_slocs_to_adresses(void) {
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

      if (!slocs[addr]) {
        slocs[addr] = malloc(sizeof (dbg_slocdef));
        slocs[addr]->file = file;
        slocs[addr]->line = line;
        //printf("Mapped 0x%04X => %s:%d\n", addr, file->name, line->line_number);
      }
      free(c_spans[i]);
    }
    free(c_spans);
  }
}

/* Get a symbol by its address, depending on current banking */
dbg_symbol *symbol_get_by_addr(int cpu, int addr, int mem, int lc) {
  char buf[6];
  if (cpu == CPU_6502) {
    if (addr < 0xD000 || addr > 0xDFFF) {
      /* There's no LC bank 2 out of this range */
      lc = 1;
    }
    if (addr < 0xC800) {
      mem = RAM;
    }
  }
  /* Hack for generated symbols with no address
   * (Thanks MAME!) */
  if (addr > 2 * STORAGE_SIZE)
    return NULL;

  /* If not offset by STORAGE_SIZE, it's either
   * in the main map of symbols, */
  else if (addr < STORAGE_SIZE &&
           symbols[addr] && 
           symbols[addr]->real_addr == addr && 
           symbols[addr]->mem == mem && 
           symbols[addr]->lcbank == lc) {
    return symbols[addr];

  /* or in the generated symbol cache ? */
  } else {
    if (addr < STORAGE_SIZE &&
        gen_sym_cache[addr][mem][lc] &&
        gen_sym_cache[addr][mem][lc]->real_addr == addr) {
        return gen_sym_cache[addr][mem][lc];
      }
    /* Symbol with no address, shifted to
     * STORAGE_SIZE => 2 * STORAGE_SIZE */
    if (addr > STORAGE_SIZE)
      return gen_symbols[addr - STORAGE_SIZE];
  }

  snprintf(buf, 6, "%04X", addr);
  return generate_symbol(buf, addr, mem, lc, NULL);
}

/* Get symbol by name */
dbg_symbol *symbol_get_by_name(const char *name) {
  int i;

  /* Either a generated one - doing this one
   * first for speed, there are less of them */
  for (i = 0; i < STORAGE_SIZE; i++) {
    dbg_symbol *symbol = gen_symbols[i];

    if (symbol && !strcmp(name, symbol->name))
      return symbol;
  }

  /* Or from the dbg file */
  for (i = 0; i < STORAGE_SIZE; i++) {
    dbg_symbol *symbol = symbols[i];

    if (symbol && !strcmp(name, symbol->name))
      return symbol;
  }

  return NULL;
}

/* Generate a fake symbol. Rather than a raw address,
 * it helps to remind us what the current banking 
 * configuration is */
dbg_symbol *generate_symbol(const char *param_name, int param_addr, int mem, int lc, const char *extra) {
  dbg_symbol *s;
  char full_name[64] = "";

  /* Unset banking according to address */
  lc = (param_addr < 0xD000 || param_addr > 0xDFFF) ? 1 : lc;
  mem = (param_addr < 0xC800) ? RAM : mem;

  /* Format the name */
  snprintf(full_name, 64, "%s.LC%d.", (mem == RAM ? "RAM":"ROM"), lc);
  strcat(full_name, param_name);
  if (extra) {
    strcat(full_name, ":");
    strcat(full_name, extra);
  }

  /* Check if we have it */
  if (param_addr > 0 && gen_sym_cache[param_addr][mem][lc]) {
    return gen_sym_cache[param_addr][mem][lc];
  } else if (param_addr == 0) {
    s = symbol_get_by_name(full_name);
    if (s) {
      return s;
    }
  }
  /* Otherwise build it */
  s = malloc(sizeof(dbg_symbol));
  s->addr = STORAGE_SIZE + num_gen_symbols;
  s->real_addr = param_addr;
  s->name = strdup(full_name);
  s->size = 0;
  s->segment = 0;
  s->mem = mem;
  s->lcbank = lc;

  /* Insert in map */
  gen_symbols[num_gen_symbols] = s;

  /* If it has an address, insert in cache */
  if (s->real_addr > 0)
    gen_sym_cache[s->real_addr][mem][lc] = s;

  /* Bump total */
  num_gen_symbols++;
  return s;
}

/* Symbol name getter */
const char *symbol_get_name(dbg_symbol *symbol) {
  if (!symbol)
    return NULL;

  return symbol->name;
}

/* Address getter */
int symbol_get_addr(dbg_symbol *symbol) {
  return symbol ? 
    (symbol->real_addr > 0 ? symbol->real_addr : symbol->addr)
      : 0;
}

/* Get source location for address */
dbg_slocdef *sloc_get_for_addr(int addr) {
  if (addr >= 0 && addr < STORAGE_SIZE)
    return slocs[addr];
  else
    return NULL;
}

/* SLOC filename getter */
const char *sloc_get_filename(dbg_slocdef *sloc) {
  return sloc ? 
          sloc->file ? sloc->file->name 
            : NULL
              : NULL;
}

/* SLOC line number getter */
int sloc_get_line(dbg_slocdef *sloc) {
  return sloc ? 
          sloc->line ? sloc->line->line_number 
            : -1
              : -1;
}
