#ifndef __symbols_h
#define __symbols_h

typedef struct _line dbg_line;
typedef struct _span dbg_span;
typedef struct _file dbg_file;
typedef struct _segment dbg_segment;
typedef struct _symbol dbg_symbol;
typedef struct _slocdef dbg_slocdef;

void load_syms(const char *file);
void load_lbls(const char *file);
void map_slocs_to_adresses(void);

dbg_symbol *symbol_get_by_addr(int addr);
const char *symbol_get_name(dbg_symbol *symbol);

dbg_slocdef *sloc_get_for_addr(int addr);
const char *sloc_get_filename(dbg_slocdef *sloc);
int sloc_get_line(dbg_slocdef *sloc);

#endif
