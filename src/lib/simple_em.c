#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <apple2enh.h>
#include <em.h>
#include "simple_em.h"

#define PAGE_SIZE 128

int em_init(void) {
  return em_install(a2_auxmem_emd);
}

void em_store (int page_num,  void *data, size_t count) {
    struct em_copy c;

    c.buf   = data;
    c.count = count;
    c.page = page_num;
    c.offs = 0;

    em_copyto (&c);
}

void *em_read (int page_num, size_t count) {
    struct em_copy c;

    c.buf   = malloc(count);
    c.count = count;
    c.page = page_num;
    c.offs = 0;

    em_copyfrom (&c);
    return c.buf;
}

void em_store_str (int page_num, char *data) {
  em_store(page_num, data, strlen(data) + 1);
}

char *em_read_str (int page_num) {
  char *data = malloc(PAGE_SIZE);
  data = em_read(page_num, PAGE_SIZE);

  return realloc(data, strlen(data) + 1);
}
