#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <apple2.h>
#include <tgi.h>
#else
#include "tgi_compat.h"
#endif
#include "extended_conio.h"

#define DATASET "IN12"
#define BUFSIZE 255

char buf[BUFSIZE];

int max_x = 0, max_y = 0;

int start_x, start_y;
int end_x, end_y;

static void read_file(FILE *fp);

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
  tgi_setcolor(TGI_COLOR_WHITE);
  while (!feof(solfp)) {
    int x, y;
    fread(&x, sizeof(int), 1, solfp);
    fread(&y, sizeof(int), 1, solfp);
    printf("(%d,%d) =>", x, y);
    tgi_setpixel(x, y);
  }
  printf("\n");
  fclose(solfp);

  cgetc();
  exit (0);
}

static void read_file(FILE *fp) {
  int i;
  while (NULL != fgets(buf, sizeof(buf), fp)) {
    *strchr(buf, '\n') = '\0';
    max_x = strlen(buf);

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
      if (buf[i] >= 'a' && buf[i] < 'g') {
        tgi_setcolor(TGI_COLOR_BLACK);
      } else if (buf[i] >= 'g' && buf[i] < 'o') {
        tgi_setcolor(TGI_COLOR_GREEN);
      } else if (buf[i] >= 'o' && buf[i] < 'u') {
        tgi_setcolor(TGI_COLOR_ORANGE);
      } else if (buf[i] >= 'u') {
        tgi_setcolor(TGI_COLOR_VIOLET);
      }
      tgi_setpixel(i, max_y);
    }
    max_y++;
  }
}
