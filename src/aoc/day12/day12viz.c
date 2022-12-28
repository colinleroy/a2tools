#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "tgi_compat.h"
#include "extended_conio.h"

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

  tgi_install(a2_hi_tgi);
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
        tgi_setcolor(TGI_COLOR_VIOLET);
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
