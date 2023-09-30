/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include <em.h>
#include "simple_em.h"

#define PAGE_SIZE 128

int em_init(void) {
  return em_install(a2e_auxmem_emd);
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
