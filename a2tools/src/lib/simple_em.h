#ifndef __simple_em_h
#define __simple_em_h

int em_init(void);
void em_store (int page_num,  void *data, size_t count);
void *em_read (int page_num, size_t count);

void em_store_str (int page_num,  char *data);
char *em_read_str (int page_num);

#endif
