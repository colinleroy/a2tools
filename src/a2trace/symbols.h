#ifndef __symbols_h
#define __symbols_h

typedef struct _line dbg_line;
typedef struct _span dbg_span;
typedef struct _file dbg_file;
typedef struct _segment dbg_segment;
typedef struct _symbol dbg_symbol;
typedef struct _slocdef dbg_slocdef;
typedef struct _mod dbg_mod;
typedef struct _scope dbg_scope;

#define STORAGE_SIZE 16777215

typedef enum _sym_type {
  EQUATES,
  NORMAL
} sym_type;


void load_syms(const char *file, char **excluded_files, char **excluded_segments);
void load_lbls(const char *file);
void map_slocs_to_adresses(char **excluded_segments);

dbg_symbol *symbol_get_by_addr(int cpu, int addr, int main, int lc);
dbg_symbol *symbol_get_by_name(const char *name, sym_type type);
int symbol_get_addr(dbg_symbol *symbol);
const char *symbol_get_name(dbg_symbol *symbol);
dbg_symbol *generate_symbol(const char *param_name, int param_addr, int main, int lc, const char *extra);

dbg_slocdef *sloc_get_for_addr(int addr);
const char *sloc_get_filename(dbg_slocdef *sloc);
int sloc_get_line(dbg_slocdef *sloc);

#endif
