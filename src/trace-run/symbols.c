#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "strsplit.h"
#include "slist.h"
#include "general.h"
#include "symbols.h"

#define STORAGE_SIZE 200000

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

struct _slocdef {
  dbg_file *file;
  dbg_line *line;
};

static dbg_line **lines = NULL;
static dbg_span **spans = NULL;
static dbg_file **files = NULL;
static dbg_segment **segs = NULL;
static dbg_symbol **symbols = NULL;
static dbg_slocdef **slocs = NULL;


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

static void allocate_data(void) {
  if (lines != NULL) {
    return;
  }

  lines      = malloc(sizeof(dbg_line *) * STORAGE_SIZE);
  spans      = malloc(sizeof(dbg_span *) * STORAGE_SIZE);
  files      = malloc(sizeof(dbg_file *) * STORAGE_SIZE);
  segs       = malloc(sizeof(dbg_segment *) * STORAGE_SIZE);
  symbols    = malloc(sizeof(dbg_symbol *) * STORAGE_SIZE);
  slocs      = malloc(sizeof(dbg_slocdef *) * STORAGE_SIZE);

  memset(lines,     0, sizeof(dbg_line *) * STORAGE_SIZE);
  memset(spans,     0, sizeof(dbg_span *) * STORAGE_SIZE);
  memset(files,     0, sizeof(dbg_file *) * STORAGE_SIZE);
  memset(segs,      0, sizeof(dbg_segment *) * STORAGE_SIZE);
  memset(symbols,   0, sizeof(dbg_symbol *) * STORAGE_SIZE);
  memset(slocs    , 0, sizeof(dbg_slocdef *) * STORAGE_SIZE);
}

void load_syms(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];

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
      free(subparts);
    }
    free(parts);
  }
  fclose(fp);
  cleanup_symbols();
}

void load_lbls(const char *file) {
  FILE *fp = fopen(file, "r");
  char buf[BUF_SIZE];

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
          symbols[addr]->size = 0;
          //printf("Added %s at %04X\n", name, addr);
        }
      }
    }
    free(parts);
  }
  fclose(fp);
  cleanup_symbols();
}

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

      if (slocs[addr]) {
        //printf("Warning: Address 0x%04x already set\n", addr);
      } else {
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

dbg_symbol *symbol_get_by_addr(int addr) {
  return symbols[addr];
}

const char *symbol_get_name(dbg_symbol *symbol) {
  return symbol ? symbol->name 
          : NULL;
}

dbg_slocdef *sloc_get_for_addr(int addr) {
  return slocs[addr];
}
const char *sloc_get_filename(dbg_slocdef *sloc) {
  return sloc ? 
          sloc->file ? sloc->file->name 
            : NULL
              : NULL;
}

int sloc_get_line(dbg_slocdef *sloc) {
  return sloc ? 
          sloc->line ? sloc->line->line_number 
            : -1
              : -1;
}
