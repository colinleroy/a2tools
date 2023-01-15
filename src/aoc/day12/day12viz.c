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
#include <errno.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "tgi_compat.h"
#include "extended_conio.h"

/* Init HGR segment */
#pragma rodata-name (push, "HGR")
const char hgr = 0;
#pragma rodata-name (pop)

#define DATASET "IN12"
#define BUFSIZE 255

char buf[BUFSIZE];

int max_x = 0, max_y = 0;

int start_x, start_y;
int end_x, end_y;

static void read_file(FILE *fp);
static void draw_solution(FILE *solfp);

int main(void) {
  FILE *fp, *solfp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  tgi_install(a2e_hi_tgi);
  tgi_init ();

  read_file(fp);

  fclose(fp);

  solfp = fopen(DATASET "OUT", "rb");
  draw_solution(solfp);
  fclose(solfp);

  cgetc();
  tgi_done();
  exit (0);
}

static int x_offset;
static int y_offset;

static void read_file(FILE *fp) {
  int i;
  y_offset = (96 - 40)/2;
  while (NULL != fgets(buf, sizeof(buf), fp)) {
    *strchr(buf, '\n') = '\0';
    max_x = strlen(buf);
    x_offset = (140 - (max_x)) / 2;

    for (i = 0; i < max_x; i++) {
      if (buf[i] == 'S') {
        buf[i] = 'a';
        end_x = i;
        end_y = max_y;
      } else if (buf[i] == 'E') {
        buf[i] = 'z';
        start_x = i;
        start_y = max_y;
      }
      if (buf[i] >= 'a' && buf[i] < 'c') {
        tgi_setcolor(TGI_COLOR_BLACK);
      } else if (buf[i] >= 'c' && buf[i] < 'l') {
        tgi_setcolor(TGI_COLOR_GREEN);
      } else if (buf[i] >= 'l' && buf[i] < 'u') {
        tgi_setcolor(TGI_COLOR_ORANGE);
      } else if (buf[i] >= 'u') {
        tgi_setcolor(TGI_COLOR_PURPLE);
      }
      tgi_setquadpixel(i + x_offset, max_y + y_offset);
    }
    max_y++;
  }

}

static void draw_solution(FILE *solfp) {

  tgi_setcolor(TGI_COLOR_WHITE);

  while (!feof(solfp)) {
    int x, y;
    fread(&x, sizeof(int), 1, solfp);
    fread(&y, sizeof(int), 1, solfp);
    printf("(%d,%d) =>", x, y);
    tgi_setquadpixel(x + x_offset, y + y_offset);
  }
}
