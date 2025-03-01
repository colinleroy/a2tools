#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

int main(int argc, char *argv[]) {
  FILE *fp;
  char *filename;
  int c;

  if (argc < 2) {
    printf("Usage: %s [input.raw]\n", argv[0]);
    exit(1);
  }

  fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    printf("Can not open %s\n", argv[1]);
    exit(1);
  }
  filename = argv[1];
  if (strchr(filename, '.')) {
    *strchr(filename, '.') = 0;
  }
  if (strchr(filename, '/')) {
    filename = strrchr(filename, '/')+1;
  }

  printf("         .export _%s_snd, _play_%s\n\n", filename, filename);
  printf("         .import _play_sample\n\n");

  printf("         .data\n\n");

  printf(".align $100\n"
         "_%s_snd:\n", filename);
  while ((c = fgetc(fp)) != EOF) {
    int r = (c*(NUM_LEVELS-1))/255, byte;
    r = (r/STEP)*STEP;
    byte = (r*2)+PAGE_CROSSER;
    if (byte > 255) {
      fprintf(stderr, "Range error - too many levels\n");
      exit(1);
    }
    printf("         .byte $%02X    ; %d * 2 + $%02X\n", byte, r, PAGE_CROSSER); /* *2 to avoid ASLing, + PAGE_CROSSER to avoid adding */
  }
  printf("         .byte (%d*2 + $%02x); play_done\n\n", NUM_LEVELS, PAGE_CROSSER);

  printf("         .code\n\n"
         "_play_%s:\n"
         "         lda #<_%s_snd\n"
         "         ldx #>_%s_snd\n"
         "         jmp _play_sample\n",
       filename, filename, filename);
  fclose(fp);
}
